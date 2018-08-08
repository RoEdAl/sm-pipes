/*
    named-pipe-server.hpp
*/

#ifndef _NAMED_PIPE_SEVER_HPP_
#define _NAMED_PIPE_SEVER_HPP_

#include "sm-pipe-name-routines.hpp"
#include "worker-thread.hpp"
#include "hres-routines.hpp"
#include "pipe-server-basics.hpp"
#include "chunked-buffer.hpp"
#include "critical-section.hpp"

template<size_t INSTANCES, size_t BUFSIZE, typename S>
class named_pipe_server :public pipe_server_basics, public worker_thread<named_pipe_server<INSTANCES,BUFSIZE,S>>
{
private:

    void init()
    {
        m_fValid = true;
        m_instanceCnt = 0;

        m_aWaitObjects[3*INSTANCES] = m_evStop;

        m_aInstance.SetCount(INSTANCES);
    }

	static const size_t NUM_HANDLES = (3 * INSTANCES) + 1;
	static const DWORD WRITE_RETRY_TIMEOUT = 50;

public:

    named_pipe_server(INotify& notifier)
        :m_notifier(notifier)
    {
        init();
        m_sFullPipeName = sm_pipe_name_routines::get_full_pipe_name(S::GetPipeName());

        for(size_t i = 0; i < INSTANCES; ++i)
        {
            CHandle evRead;
			CHandle evWrite;
			CHandle evNewMsg;

            CNamedPipe pipe;

            create_event(evRead);
			create_event(evWrite, false);
			create_event(evNewMsg, false);

            create_named_pipe(pipe);

            m_aWaitObjects[3*i] = evRead;
			m_aWaitObjects[(3 * i)+1] = evWrite;
			m_aWaitObjects[(3 * i) + 2] = evNewMsg;

            CPipeInstance* pPipeInstance = new CPipeInstance(++m_instanceCnt, pipe, evRead, evWrite, evNewMsg, notifier);
            m_aInstance.GetAt(i).Attach(pPipeInstance);
        }
    }

    named_pipe_server(S& securityPolicy, INotify& notifier)
		:m_notifier(notifier)
    {
        init();

        bool fCustomSecurityAttr = false;
        CSecurityDesc sd;
        CSecurityAttributes sa;

        if(!securityPolicy.Init())
        {
            m_fValid = false;
            return;
        }

        m_sFullPipeName = sm_pipe_name_routines::get_full_pipe_name(securityPolicy.GetPipeName());

        if(securityPolicy.BuildSecurityDesc(sd))
        {
            sa.Set(sd);
            fCustomSecurityAttr = true;
        }

        for(size_t i = 0; i < INSTANCES; ++i)
        {
			CHandle evRead;
			CHandle evWrite;
			CHandle evNewMsg;

            CNamedPipe pipe;

			create_event(evRead);
			create_event(evWrite, false);
			create_event(evNewMsg, false);

            create_named_pipe(pipe, fCustomSecurityAttr, sa);

			m_aWaitObjects[3 * i] = evRead;
			m_aWaitObjects[(3 * i) + 1] = evWrite;
			m_aWaitObjects[(3 * i) + 2] = evNewMsg;

            CPipeInstance* pPipeInstance = new CPipeInstance(++m_instanceCnt, pipe, evRead, evWrite, evNewMsg, notifier);
            m_aInstance.GetAt(i).Attach(pPipeInstance);
        }
    }

    bool IsValid() const
    {
        return m_fValid;
    }

	const CString& GetPipeName() const
	{
		return m_sFullPipeName;
	}

	bool SendMessage(INSTANCENO instanceNo, const Buffer& msg)
	{
		for (size_t i = 0; i < INSTANCES; ++i)
		{
			if (m_aInstance[i]->GetInstanceNo() != instanceNo) continue;
			m_aInstance[i]->SendMessage(msg);
			return true;
		}

		return false;
	}

    void ThreadProc()
    {
		using namespace hres_routines;

        ATLASSERT(IsValid());

        connect_pipes();

        bool bStop = false;
		DWORD dwTimeout = INFINITE;

        while(!bStop)
        {
            DWORD dwWait = ::WaitForMultipleObjects(NUM_HANDLES, m_aWaitObjects, false, dwTimeout);
			dwTimeout = INFINITE;

            if(dwWait == WAIT_FAILED)
            {
                bStop = true;
                AtlThrowLastWin32();
                continue;
            }

            switch(dwWait)
            {
                case WAIT_OBJECT_0 + NUM_HANDLES - 1: // m_evStop
                bStop = true;
                break;

				case WAIT_TIMEOUT:
					for_each_instance([&dwTimeout](auto& instance) {instance.Run(WAIT_TIMEOUT, dwTimeout); });
					break;

                default: // signal from pipe instance
                {
					if (dwWait < WAIT_OBJECT_0 || dwWait >= (WAIT_OBJECT_0 + NUM_HANDLES))
					{
						// index out of range
						bStop = true;
						AtlThrow(HRESULT_FROM_WIN32(ERROR_INVALID_INDEX));
						break;
					}

                    int i = dwWait - WAIT_OBJECT_0;
					int nInstance = i / 3;
					int nEvent = i % 3;
					ATLASSERT(nInstance >= 0 && nInstance < INSTANCES);

                    CAutoPtr<CPipeInstance>& pPipe = m_aInstance[nInstance];
                    _ATLTRY
                    {
                        pPipe->Run(nEvent, dwTimeout);
                    }
                    _ATLCATCH(e)
                    {
                        if(win32_error<ERROR_BROKEN_PIPE>(e.m_hr))
                        {
                            pPipe->DisconnectAndReconnect(++m_instanceCnt);
                        }
                    }

                    break;
                }
            }
        }

        disconnect_pipes();
    }

private:

    void connect_pipes()
    {
        for_each_instance([](auto& instance) {instance.ConnectToNewClient();});
    }

    void disconnect_pipes()
    {
        HANDLE aHandle[2*INSTANCES];
        DWORD dwCnt = 0;

        for_each_instance([&aHandle, &dwCnt](auto& instance) {
            HANDLE h = instance.CancelPendingConnectOrRead();
            if(h != nullptr) aHandle[dwCnt++] = h;
			h = instance.CancelPendingWriteOperation();
			if (h!=nullptr) aHandle[dwCnt++] = h;
        });

		if (dwCnt > 0)
		{
			DWORD dwWait = ::WaitForMultipleObjects(dwCnt, aHandle, true, INFINITE);
			ATLASSERT(dwWait >= WAIT_OBJECT_0 && dwWait < (WAIT_OBJECT_0 + dwCnt));
		}

        for_each_instance([](auto& instance) {instance.FinishCancelingAndDisconnect();});
    }

    class CPipeInstance
    {
    public:

		CPipeInstance(INSTANCENO instanceNo, CNamedPipe& pipeInstance,
			CHandle& evRead, CHandle& evWrite, CHandle& evNewMsg,
			INotify& notifier)
			:m_instanceNo(instanceNo), m_pipeInstance(pipeInstance),
			m_evRead(evRead), m_evWrite(evWrite), m_evNewMsg(evNewMsg),
			m_notifier(notifier),
			m_eState(INSTANCE_STATE_CONNECTING),
			m_fPendingIO(false), m_fPendingWrite(false)
        {
            ZeroMemory(&m_ovRead, sizeof(OVERLAPPED));
			m_ovRead.hEvent = m_evRead;

			ZeroMemory(&m_ovWrite, sizeof(OVERLAPPED));
			m_ovWrite.hEvent = m_evWrite;
        }

		INSTANCENO GetInstanceNo() const
		{
			return m_instanceNo;
		}

		void SendMessage(const Buffer& msg)
		{
			{
				CCriticalSectionLocker locker(m_cs);
				m_outputBuffers.AddTail(msg);
			}
			set_event(m_evNewMsg);
		}

        void ConnectToNewClient()
        {
            HRESULT hRes = m_pipeInstance.ConnectNamedPipe(&m_ovRead);

            switch(hRes)
            {
                case __HRESULT_FROM_WIN32(ERROR_IO_PENDING):
                m_fPendingIO = true;
                break;

                case __HRESULT_FROM_WIN32(ERROR_PIPE_CONNECTED):
                set_event(m_evRead);
                m_notifier.OnConnect(m_instanceNo);
                break;

                default:
                AtlThrow(hRes);
                break;
            }

            m_eState = m_fPendingIO ? INSTANCE_STATE_CONNECTING : INSTANCE_STATE_READING;
        }

        void DisconnectAndReconnect(INSTANCENO newInstanceNo)
        {
			HANDLE aHandle[2], h;
			DWORD dwCnt = 0;

			h = CancelPendingConnectOrRead();
			if (h != nullptr) aHandle[dwCnt++] = h;
			h = CancelPendingWriteOperation();
			if (h != nullptr) aHandle[dwCnt++] = h;

			if (dwCnt > 0)
			{
				DWORD dwWait = ::WaitForMultipleObjects(dwCnt, aHandle, true, INFINITE);
				ATLASSERT(dwWait >= WAIT_OBJECT_0 && dwWait < (WAIT_OBJECT_0 + dwCnt));
			}

			FinishCancelingAndDisconnect();

            m_eState = INSTANCE_STATE_CONNECTING;
            m_instanceNo = newInstanceNo;
            m_inputBuffer.Clear();

            ConnectToNewClient();
        }

        HANDLE CancelPendingConnectOrRead()
        {
            if(m_fPendingIO)
            {
                HRESULT hRes = m_pipeInstance.CancelIoEx(&m_ovConnect);
                if(SUCCEEDED(hRes))
                {
                    return m_ovConnect.hEvent;
                }
                else
                {
                    AtlThrow(hRes);
                }
            }

            return nullptr;
        }

		HANDLE CancelPendingWriteOperation()
		{
			if (m_fPendingWrite)
			{
				HRESULT hRes = m_pipeInstance.CancelIoEx(&m_ovWrite);
				if (SUCCEEDED(hRes))
				{
					return m_evWrite;
				}
				else
				{
					AtlThrow(hRes);
				}
			}

			return nullptr;
		}

        HRESULT FinishCancelingAndDisconnect()
        {
			using namespace hres_routines;

            HRESULT hRes = S_FALSE;
			DWORD dwBytesTransfered;

			if (m_fPendingWrite)
			{
				hRes = m_pipeInstance.GetOverlappedResult(&m_ovRead, dwBytesTransfered, true);
				if (abort_succeeded(hRes))
				{
					m_fPendingWrite = false;
				}
			}

            if(m_fPendingIO)
            {
                hRes = m_pipeInstance.GetOverlappedResult(&m_ovConnect, dwBytesTransfered, true);
                if(abort_succeeded(hRes))
                {
                    m_fPendingIO = false;
                }
            }

			if (m_eState != INSTANCE_STATE_CONNECTING)
			{
				hRes = m_pipeInstance.DisconnectNamedPipe();
				m_notifier.OnDisconnect(m_instanceNo);
			}

            return hRes;
        }

        void Run(int nEvent, DWORD& dwTimeout)
        {
			using namespace hres_routines;

			switch (nEvent)
			{
				case 0: // evRead or Connect
					if (m_fPendingIO)
					{
						DWORD dwBytesTransfered;

						m_fPendingIO = false;
						HRESULT hRes = m_pipeInstance.GetOverlappedResult(&m_ovRead, dwBytesTransfered, true);
						if (!read_succeeded(hRes))
						{
							AtlThrow(hRes);
						}

						switch (m_eState)
						{
						case INSTANCE_STATE_CONNECTING:
							m_eState = INSTANCE_STATE_READING;
							m_notifier.OnConnect(m_instanceNo);
							break;

						case INSTANCE_STATE_READING:
							m_inputBuffer.TrimLastChunk(dwBytesTransfered);
							if (!more_data(hRes))
							{
								message_completed();
							}
							break;
						}
					}

					sync_or_async_read();
					break;

				case 1: // evWrite
				{
					DWORD dwBytesTransferred;
					m_fPendingWrite = false;
					HRESULT hRes = m_pipeInstance.GetOverlappedResult(&m_ovWrite, dwBytesTransferred, true);
				}

				case 2: // evNewMsg
				case 3: // timeout
					sync_or_async_write(dwTimeout);
					break;
			}
        }

    protected:

		INSTANCENO m_instanceNo;
		INSTANCE_STATE m_eState;
		INotify& m_notifier;

		union
		{
			struct
			{
				OVERLAPPED m_ovConnect;
				bool m_fPendingIO;
			};
			struct
			{
				OVERLAPPED m_ovRead;
				bool m_fPendingRead;
			};
		};

		OVERLAPPED m_ovWrite;
		bool m_fPendingWrite;

        CHandle m_evRead;
		CHandle m_evWrite;
		CHandle m_evNewMsg;
        
        CNamedPipe m_pipeInstance;

        CChunkedBuffer m_inputBuffer;

		CCriticalSection m_cs; // access to m_outputBuffers
		CAtlList<CMemoryBlock> m_outputBuffers;
		CMemoryBlock m_currentMemoryBlock;

	private:

		enum GET_OUT_BUFFER_RESULT
		{
			BUFFER_RETURNED,
			NO_MORE_BUFFERS,
			CANNOT_LOCK_BUFFER_LIST
		};

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

		void sync_or_async_read()
		{
			using namespace hres_routines;

			while (true)
			{
				DWORD dwBytesTransfered, dwBytesLeftInThisMsg;

				HRESULT hRes = m_pipeInstance.PeekNamedPipe(dwBytesLeftInThisMsg);
				if (SUCCEEDED(hRes))
				{
					size_t nMsgLength = m_inputBuffer.GetCount() + dwBytesLeftInThisMsg;
					bool fMsgTooLong = nMsgLength >= MAX_INPUT_MESSAGE_SIZE;
					Buffer& buffer = prepare_input_buffer(fMsgTooLong, dwBytesLeftInThisMsg);
					hRes = m_pipeInstance.ReadToArray(buffer, &m_ovRead);
					if (read_succeeded(hRes))
					{
						hRes = m_pipeInstance.GetOverlappedResult(&m_ovRead, dwBytesTransfered, true);
						if (read_succeeded(hRes))
						{
							if (fMsgTooLong)
							{ // just ignore this message silently
								m_inputBuffer.Clear();
							}
							else
							{
								m_inputBuffer.TrimLastChunk(dwBytesTransfered);
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
						m_fPendingIO = true;
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
					HRESULT hRes = m_pipeInstance.Write(m_currentMemoryBlock.GetData(), static_cast<DWORD>(m_currentMemoryBlock.GetSize()), &m_ovWrite);
					if (SUCCEEDED(hRes))
					{
						DWORD dwBytesTransferred;
						hRes = m_pipeInstance.GetOverlappedResult(&m_ovWrite, dwBytesTransferred, true);
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
			m_notifier.OnMessage(m_instanceNo, buffer);
			m_inputBuffer.Clear();
		}
    };

private:

    bool m_fValid;
    CString m_sFullPipeName;
    INSTANCENO m_instanceCnt;
    HANDLE m_aWaitObjects[NUM_HANDLES];
    CAutoPtrArray<CPipeInstance> m_aInstance;
    INotify& m_notifier;

protected:

    template<typename O>
    void for_each_instance(O operation)
    {
        for(size_t i = 0; i < INSTANCES; ++i)
        {
            operation(*(m_aInstance[i].m_p));
        }
    }

    void create_named_pipe(CNamedPipe& pipe) const
    {
        HRESULT hRes = pipe.CreateNamedPipe(m_sFullPipeName,
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            INSTANCES, BUFSIZE, BUFSIZE, PIPE_CONNECT_TIMEOUT,
            nullptr);

        if(FAILED(hRes))
        {
            AtlThrow(hRes);
        }
    }

    void create_named_pipe(CNamedPipe& pipe, bool bCustonSecurityAttr, CSecurityAttributes& sa) const
    {
        HRESULT hRes = pipe.CreateNamedPipe(m_sFullPipeName,
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            INSTANCES, BUFSIZE, BUFSIZE, PIPE_CONNECT_TIMEOUT,
            const_cast<CSecurityAttributes *>(bCustonSecurityAttr? &sa : nullptr));

        if(FAILED(hRes))
        {
            AtlThrow(hRes);
        }
    }
};

#endif // _NAMED_PIPE_SEVER_HPP_
