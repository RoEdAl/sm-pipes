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
#include "pipe-client-basics.hpp"

template<size_t BUFSIZE>
class named_pipe_client :public pipe_client_basics, public worker_thread<named_pipe_client<BUFSIZE>>
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
	CMemoryBlock m_currentMemoryBlock;

	CChunkedBuffer m_inputBuffer;

	CString m_sPipe;
	CNamedPipe m_pipe;
	bool m_fPendingRead;
	bool m_fPendingWrite;

	INotify& m_notifier;

	enum GET_OUT_BUFFER_RESULT
	{
		BUFFER_RETURNED,
		NO_MORE_BUFFERS,
		CANNOT_LOCK_BUFFER_LIST
	};

public:

	named_pipe_client(const CString& sPipe, INotify& notifier)
		:m_sPipe(sPipe), m_fPendingRead(false), m_fPendingWrite(false), m_notifier(notifier)
	{
		create_event(m_evRead);
		create_event(m_evWrite, false);
		create_event(m_evNewMsg, false);

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

    void SendMessage(const void* pBlock, size_t nSize)
    {
        {
            CMemoryBlock mb(pBlock, nSize);

            CCriticalSectionLocker locker(m_cs);
            POSITION pos = m_outputBuffers.AddTail();

            void* _pBlock;
            size_t _nSize;
            mb.Detach(_pBlock, _nSize);

            m_outputBuffers.GetAt(pos).Attach(_pBlock, _nSize);
        }
        set_event(m_evNewMsg);
    }

	const CString& GetPipeName() const
	{
		return m_sPipe;
	}

	void ThreadProc()
	{
		bool fStop = false;
		while (!fStop)
		{
			_ATLTRY
			{
				open_pipe();
				set_message_mode();
				main_loop();
				fStop = true;
			}
			_ATLCATCH(e)
			{
				reset(!is_pipe_broken(e));

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
		using namespace hres_routines;

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

				sync_or_async_read();
			}
			break;

			case WAIT_OBJECT_0 + 1: // evWrite
			{
				DWORD dwBytesTransferred;
				m_fPendingWrite = false;
				HRESULT hRes = m_pipe.GetOverlappedResult(&m_ovWrite, dwBytesTransferred, true);
			}

			case WAIT_OBJECT_0 + 2: // evNewMsg
			case WAIT_TIMEOUT: // try again
				sync_or_async_write(dwTimeout);
				break;

			case 3: // evStop
				fStop = true;
				break;

			}
		}

		reset(true);
	}

	void reset(bool fOrigin)
	{
		using namespace hres_routines;

		if (m_pipe.m_h != nullptr)
		{
			HANDLE ahWait[2];
			DWORD dwCnt = 0, dwBytesTransferred;
			HRESULT hRes;

			if (m_fPendingRead)
			{
				ahWait[dwCnt++] = m_evRead;
				hRes = m_pipe.CancelIoEx(&m_ovRead);
				ATLASSERT(SUCCEEDED(hRes));
			}

			if (m_fPendingWrite)
			{
				ahWait[dwCnt++] = m_evWrite;
				hRes = m_pipe.CancelIoEx(&m_ovWrite);
				ATLASSERT(SUCCEEDED(hRes));
			}

			if (dwCnt > 0)
			{
				DWORD dwWait = ::WaitForMultipleObjects(dwCnt, ahWait, true, INFINITE);
				ATLASSERT(dwWait >= WAIT_OBJECT_0 && dwWait < (WAIT_OBJECT_0 + dwCnt));
			}

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
			m_notifier.OnDisconnect(fOrigin);
		}

		reset_event(m_evRead);

		m_inputBuffer.Clear();
		m_outputBuffers.RemoveAll();
		m_currentMemoryBlock.Empty();
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

		m_notifier.OnConnect();
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

	GET_OUT_BUFFER_RESULT get_out_buffer()
	{
		CCriticalSectionSoftLocker locker(m_cs);
		if (!locker.IsLocked()) return CANNOT_LOCK_BUFFER_LIST;

		size_t nCount = m_outputBuffers.GetCount();
		if (nCount == 0)
		{
			m_currentMemoryBlock.Empty();
			return NO_MORE_BUFFERS;
		}

		m_currentMemoryBlock = const_cast<const CAtlList<CMemoryBlock>&>(m_outputBuffers).GetHead();
		m_outputBuffers.RemoveHeadNoReturn();
		return BUFFER_RETURNED;
	}

	void sync_or_async_read()
	{
		using namespace hres_routines;

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

	void sync_or_async_write(DWORD& dwTimeout)
	{
		using namespace hres_routines;

		bool fWrite = true;
		while (fWrite)
		{
			GET_OUT_BUFFER_RESULT eRes = get_out_buffer();
			switch (eRes)
			{
			case BUFFER_RETURNED:
			{
				HRESULT hRes = m_pipe.Write(m_currentMemoryBlock.GetData(), static_cast<DWORD>(m_currentMemoryBlock.GetSize()), &m_ovWrite);
				if (SUCCEEDED(hRes))
				{
					DWORD dwBytesTransferred;
					hRes = m_pipe.GetOverlappedResult(&m_ovWrite, dwBytesTransferred, true);
					if (SUCCEEDED(hRes))
					{
						m_fPendingWrite = false;
					}
					else
					{
						AtlThrow(hRes);
					}
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
				dwTimeout = WRITE_RETRY_TIMEOUT;
				fWrite = false;
				break;

			default:
				fWrite = false;
				break;
			}
		}
	}

	void message_completed()
	{
		Buffer buffer;
		m_inputBuffer.GetData(buffer);
		m_notifier.OnMessage(buffer);
		m_inputBuffer.Clear();
	}

    static bool is_pipe_broken(HRESULT hRes)
    {
        switch(hRes)
        {
            case __HRESULT_FROM_WIN32(ERROR_BROKEN_PIPE):
            case __HRESULT_FROM_WIN32(ERROR_PIPE_NOT_CONNECTED):
            return true;

            default:
            return false;
        }
    }
};

#endif