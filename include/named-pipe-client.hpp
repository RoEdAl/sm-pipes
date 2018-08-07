/*
	named-pipe-client.hpp
*/

#ifndef _NAMED_PIPE_CLIENT_HPP_
#define _NAMED_PIPE_CLIENT_HPP_

#include "worker-thread.hpp"
#include "named-pipe.hpp"
#include "hres-routines.hpp"
#include "critical-section.hpp"
#include "chunked-buffer.hpp"

template<size_t BUFSIZE>
class named_pipe_client :public worker_thread<named_pipe_client<BUFSIZE>>
{
private:

	CHandle m_evRead;
	CHandle m_evWrite;
	CHandle m_evNewMsg;

	OVERLAPPED m_ovRead;
	OVERLAPPED m_ovWrite;

	HANDLE m_aWaitObj[4];

	CCriticalSection m_cs; // access to m_outputBuffers
	CAtlList<CMemoryBlock> m_outputBuffers;

	CChunkedBuffer m_inputBuffer;

	CString m_sPipe;
	CNamedPipe m_pipe;
	bool m_fPendingRead;
	bool m_fPendingWrite;

	enum GET_OUT_BUFFER_RESULT
	{
		BUFFER_RETURNED,
		NO_MORE_BUFFERS,
		CANNOT_LOCK_BUFFER_LIST
	};

private:

	GET_OUT_BUFFER_RESULT get_out_buffer(CMemoryBlock& memory_block)
	{
		CCriticalSectionSoftLocker locker(m_cs);
		if (!locker.IsLocked()) return CANNOT_LOCK_BUFFER_LIST;

		size_t nCount = m_outputBuffers.GetCount();
		if (nCount == 0)
		{
			return NO_MORE_BUFFERS;
		}

		memory_block = const_cast<const CAtlList<CMemoryBlock>&>(m_outputBuffers).GetHead();
		m_outputBuffers.RemoveHeadNoReturn();
		return BUFFER_RETURNED;
	}

public:

	static const size_t MAX_INPUT_MESSAGE_SIZE = 1024 * 1024 * 1024;
	static const DWORD CONNECT_TIMEOUT = 1000;

	named_pipe_client(const CString& sPipe)
		:m_sPipe(sPipe), m_fPendingRead(false), m_fPendingWrite(false)
	{
		create_event(m_evRead);
		create_event(m_evWrite);
		create_event(m_evNewMsg);

		ZeroMemory(&m_ovRead, sizeof(OVERLAPPED));
		m_ovRead.hEvent = m_evRead;

		ZeroMemory(&m_ovWrite, sizeof(OVERLAPPED));
		m_ovWrite.hEvent = m_evWrite;

		m_aWaitObj[0] = m_evRead;
		m_aWaitObj[1] = m_evWrite;
		m_aWaitObj[2] = m_evNewMsg;
		m_aWaitObj[3] = m_evStop;
	}

	void SendMessage(const Buffer& msg)
	{
		{
			CCriticalSectionLocker locker(m_cs);
			m_outputBuffers.AddTail(msg);
		}
		set_event(m_evNewMsg);
	}

	void ThreadProc()
	{
		using namespace hres_routines;

		bool fStop = false;
		while (!fStop)
		{
			_ATLTRY
			{
				open_pipe();
				set_message_mode();
				main_loop();
				return;
			}
			_ATLCATCH(e)
			{
				HRESULT hRes = e;
				cancel_pending_operations();

				DWORD dwWait = ::WaitForSingleObject(m_evStop, CONNECT_TIMEOUT);
				switch (dwWait)
				{
					case WAIT_OBJECT_0 + 0:
					default:
					fStop = true;
					break;

					case WAIT_TIMEOUT:
					break;
				}
			}
		}
	}

private:

	void main_loop()
	{
		bool fStop = false;
		DWORD dwTimeout = INFINITE;
		while (!fStop)
		{
			DWORD dwWait = ::WaitForMultipleObjects(4, m_aWaitObj, false, dwTimeout);
			dwTimeout = INFINITE;

			switch (dwWait)
			{
			case WAIT_OBJECT_0 + 0: // evRead
			{
				if (m_fPendingRead)
				{
					DWORD dwBytesTransferred;
					HRESULT hRes = m_pipe.GetOverlappedResult(&m_ovRead, dwBytesTransferred, true);
					m_fPendingRead = false;
					if (!read_succeeded(hRes))
					{
						AtlThrow(hRes);
					}
					m_inputBuffer.TrimLastChunk(dwBytesTransferred);
					if (!more_data(hRes))
					{
						message_completed();
					}
				}

				// read more
				while (true)
				{
					DWORD dwBytesTransferred, dwBytesLeftInThisMsg;

					HRESULT hRes = m_pipe.PeekNamedPipe(dwBytesLeftInThisMsg);
					if (SUCCEEDED(hRes))
					{
						size_t nMsgLength = m_inputBuffer.GetCount() + dwBytesLeftInThisMsg;
						bool fMsgTooLong = nMsgLength >= MAX_INPUT_MESSAGE_SIZE;
						Buffer& buffer = prepare_input_buffer(fMsgTooLong, dwBytesLeftInThisMsg);
						hRes = m_pipe.ReadToArray(buffer, &m_ovRead);
						if (read_succeeded(hRes))
						{
							hRes = m_pipe.GetOverlappedResult(&m_ovRead, dwBytesTransferred, true);
							if (read_succeeded(hRes))
							{
								if (fMsgTooLong)
								{ // just ignore this message silently
									m_inputBuffer.Clear();
								}
								else
								{
									m_inputBuffer.TrimLastChunk(dwBytesTransferred);
									if (!more_data(hRes))
									{
										message_completed();
									}
								}
							}
							else
							{
								AtlThrow(hRes);
							}
						}
						else if (win32_error<ERROR_IO_PENDING>(hRes))
						{
							m_fPendingRead = true;
							break;
						}
						else
						{
							AtlThrow(hRes);
						}
					}
					else
					{
						AtlThrow(hRes);
					}
				}
			}
			break;

			case WAIT_OBJECT_0 + 1: // evWrite
			{
				DWORD dwBytesTransferred;
				HRESULT hRes = m_pipe.GetOverlappedResult(&m_ovWrite, dwBytesTransferred, true);
				m_fPendingWrite = false;
				reset_event(m_evWrite);
			}

			case WAIT_OBJECT_0 + 2: // evNewMsg
			case WAIT_TIMEOUT:
			{
				if (dwWait == (WAIT_OBJECT_0 + 2))
				{
					reset_event(m_evNewMsg);
				}

				bool fWrite = true;
				while (fWrite)
				{
					CMemoryBlock memory_block;
					GET_OUT_BUFFER_RESULT eRes = get_out_buffer(memory_block);
					switch (eRes)
					{
					case BUFFER_RETURNED:
					{
						HRESULT hRes = m_pipe.Write(memory_block.GetData(), static_cast<DWORD>(memory_block.GetSize()), &m_ovWrite);
						if (SUCCEEDED(hRes))
						{
							m_fPendingWrite = false;
						}
						else if (win32_error<ERROR_IO_PENDING>(hRes))
						{
							m_fPendingWrite = true;
							fWrite = false;
						}
						else
						{
							AtlThrow(hRes);
						}
					}
					break;

					case CANNOT_LOCK_BUFFER_LIST:
						dwTimeout = 50;
						fWrite = false;
						break;

					default:
						fWrite = false;
						break;
					}
				}
				break;
			}

			case 3: // evStop
				fStop = true;
				break;

			}
		}

		cancel_pending_operations();
	}

	void cancel_pending_operations()
	{
		if (m_pipe.m_h == nullptr)
		{
			return;
		}

		HANDLE ahWait[2];
		DWORD dwCnt = 0;

		if (m_fPendingRead)
		{
			ahWait[dwCnt++] = m_evRead;
			m_pipe.CancelIoEx(&m_ovRead);
		}

		if (m_fPendingWrite)
		{
			ahWait[dwCnt++] = m_evWrite;
			m_pipe.CancelIoEx(&m_ovWrite);
		}

		DWORD dwWait = ::WaitForMultipleObjects(dwCnt, ahWait, true, INFINITE);
		ATLASSERT(dwWait >= WAIT_OBJECT_0 && dwWait < (WAIT_OBJECT_0 + dwCnt));

		HRESULT hRes;
		DWORD dwBytesTransferred;

		if (m_fPendingRead)
		{
			hRes = m_pipe.GetOverlappedResult(&m_ovRead, dwBytesTransferred, true);
			if (abort_succeeded(hRes))
			{
				m_fPendingRead = false;
			}
		}

		if (m_fPendingWrite)
		{
			hRes = m_pipe.GetOverlappedResult(&m_ovWrite, dwBytesTransferred, true);
			if (abort_succeeded(hRes))
			{
				m_fPendingWrite = false;
			}
		}

		m_pipe.Close();

		m_inputBuffer.Clear();
		m_outputBuffers.RemoveAll();
	}

	void open_pipe()
	{
		using namespace hres_routines;

		while (true)
		{
			HRESULT hRes = m_pipe.Create(m_sPipe, GENERIC_READ | GENERIC_WRITE, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED);
			if (SUCCEEDED(hRes))
			{ 
				break;
			}
			else if (win32_error<ERROR_PIPE_BUSY>(hRes))
			{
				if (!::WaitNamedPipe(m_sPipe, NMPWAIT_USE_DEFAULT_WAIT))
				{
					AtlThrowLastWin32();
				}

				continue;
			}
			else
			{
				AtlThrow(hRes);
			}
		}

		m_fPendingRead = false;
		set_event(m_evRead);
	}

	void set_message_mode()
	{
		HRESULT hRes =  m_pipe.SetNamedPipeHandleState(PIPE_READMODE_MESSAGE);
		if (FAILED(hRes))
		{
			AtlThrow(hRes);
		}
	}

	Buffer& prepare_input_buffer(bool fMsgTooLong, DWORD dwBytesLeftInThisMsg)
	{
		if (fMsgTooLong)
		{
			return m_inputBuffer.GetLastChunk(BUFSIZE);
		}
		else
		{
			return m_inputBuffer.AddBuffer(dwBytesLeftInThisMsg > 0 ? dwBytesLeftInThisMsg : BUFSIZE);
		}
	}

	void message_completed()
	{
		Buffer buffer;
		m_inputBuffer.GetData(buffer);
		// TODO: Notify
		m_inputBuffer.Clear();
	}
};

#endif