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

template<size_t INSTANCES, size_t BUFSIZE, typename S>
class named_pipe_server :public pipe_server_basics, public worker_thread<named_pipe_server<INSTANCES,BUFSIZE,S>>
{
private:

    void init()
    {
        m_fValid = true;
        m_instanceCnt = 0;

        create_event(m_evWrite);

        m_aWaitObjects[INSTANCES] = m_evStop;
        m_aWaitObjects[INSTANCES + 1] = m_evWrite;

        m_aInstance.SetCount(INSTANCES);
    }

public:

    named_pipe_server(INotify& notifier)
        :m_notifier(notifier)
    {
        init();
        m_sFullPipeName = sm_pipe_name_routines::get_full_pipe_name(S::GetPipeName());

        for(size_t i = 0; i < INSTANCES; ++i)
        {
            CHandle ev;
            CNamedPipe pipe;

            create_event(ev);
            create_named_pipe(pipe);

            m_aWaitObjects[i] = ev.m_h;

            CPipeInstance* pPipeInstance = new CPipeInstance(++m_instanceCnt, pipe, ev, notifier);
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
            CHandle ev;
            CNamedPipe pipe;

            create_event(ev);
            create_named_pipe(pipe, fCustomSecurityAttr, sa);

            m_aWaitObjects[i] = ev.m_h;

            CPipeInstance* pPipeInstance = new CPipeInstance(++m_instanceCnt, pipe, ev, notifier);
            m_aInstance.GetAt(i).Attach(pPipeInstance);
        }
    }

    bool IsValid() const
    {
        return m_fValid;
    }

    void ThreadProc()
    {
        ATLASSERT(IsValid());
        using namespace hres_routines;

        connect_pipes();

        bool bStop = false;

        while(!bStop)
        {
            DWORD dwWait = ::WaitForMultipleObjects(INSTANCES + 2, m_aWaitObjects, false, INFINITE);

            if(dwWait == WAIT_FAILED)
            {
                bStop = true;
                AtlThrowLastWin32();
                continue;
            }

            switch(dwWait - WAIT_OBJECT_0)
            {
                case INSTANCES: // m_evStop
                bStop = true;
                break;

                case INSTANCES + 1: // m_evWrite
                // TODO
                break;

                default: // signal from pipe instance
                {
                    int i = dwWait - WAIT_OBJECT_0;
                    if((i < 0) || (i >= INSTANCES))
                    {
                        // index out of range
                        bStop = true;
                        AtlThrow(HRESULT_FROM_WIN32(ERROR_INVALID_INDEX));
                        break;
                    }

                    CAutoPtr<CPipeInstance>& pPipe = m_aInstance[i];
                    _ATLTRY
                    {
                        pPipe->Run();
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
        HANDLE aHandle[INSTANCES];
        DWORD dwCnt = 0;

        for_each_instance([&aHandle, &dwCnt](auto& instance) {
            HANDLE h = instance.CancelPendingOperation();
            if(h != nullptr) aHandle[dwCnt++] = h;
        });

        DWORD dwRes = ::WaitForMultipleObjects(dwCnt, aHandle, true, INFINITE);
        ATLASSERT(dwRes >= WAIT_OBJECT_0 && dwRes < (WAIT_OBJECT_0 + dwCnt));

        for_each_instance([](auto& instance) {instance.FinishCancelingAndDisconnect();});
    }

    class CPipeInstance
    {
    public:

        CPipeInstance(INSTANCENO instanceNo, CNamedPipe& pipeInstance, CHandle& evOverlap, INotify& notifier)
            :m_instanceNo(instanceNo), m_pipeInstance(pipeInstance), m_evOverlap(evOverlap), m_notifier(notifier),
             m_eState(INSTANCE_STATE_CONNECTING), m_fPendingIO(false)
        {
            ZeroMemory(&m_oOverlap, sizeof(OVERLAPPED));
            m_oOverlap.hEvent = m_evOverlap;
        }

        void ConnectToNewClient()
        {
            HRESULT hRes = m_pipeInstance.ConnectNamedPipe(&m_oOverlap);

            switch(hRes)
            {
                case __HRESULT_FROM_WIN32(ERROR_IO_PENDING):
                m_fPendingIO = true;
                break;

                case __HRESULT_FROM_WIN32(ERROR_PIPE_CONNECTED):
                set_event(m_evOverlap);
                m_notifier.OnConnect(m_instanceNo);
                break;

                default:
                AtlThrow(hRes);
                break;
            }

            m_eState = m_fPendingIO ? INSTANCE_STATE_CONNECTING : INSTANCE_STATE_READING;
        }

        void DisconnectAndReconnect(INSTANCENO instanceNo)
        {
            ATLASSERT(!m_fPendingIO);
            HRESULT hRes = m_pipeInstance.DisconnectNamedPipe();
            ATLASSUME(SUCCEEDED(hRes));
            m_notifier.OnDisconnect(m_instanceNo);

            m_eState = INSTANCE_STATE_CONNECTING;
            m_fPendingIO = false;
            m_instanceNo = instanceNo;
            m_inputBuffer.Clear();
            m_outputBuffer.Clear();

            ConnectToNewClient();
        }

        HANDLE CancelPendingOperation()
        {
            if(m_fPendingIO)
            {
                HRESULT hRes = m_pipeInstance.CancelIoEx(&m_oOverlap);
                if(SUCCEEDED(hRes))
                {
                    return m_evOverlap;
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

            if(m_fPendingIO)
            {
                DWORD dwBytesTransfered;
                hRes = m_pipeInstance.GetOverlappedResult(&m_oOverlap, dwBytesTransfered, true);
                if(abort_succeeded(hRes))
                {
                    m_fPendingIO = false;
                    if(m_eState != INSTANCE_STATE_CONNECTING)
                    {
                        m_notifier.OnDisconnect(m_instanceNo);
                    }
                }
            }

			if (m_eState != INSTANCE_STATE_CONNECTING && hRes == S_OK)
			{
				hRes = m_pipeInstance.DisconnectNamedPipe();
			}

            return hRes;
        }

        void Run()
        {
            using namespace hres_routines;

            if(m_fPendingIO)
            {
                DWORD dwBytesTransfered;

                m_fPendingIO = false;
                HRESULT hRes = m_pipeInstance.GetOverlappedResult(&m_oOverlap, dwBytesTransfered, true);
                if(!read_succeeded(hRes))
                {
                    AtlThrow(hRes);
                }

                switch(m_eState)
                {
                    case INSTANCE_STATE_CONNECTING:
                    m_eState = INSTANCE_STATE_READING;
                    m_notifier.OnConnect(m_instanceNo);
                    break;

                    case INSTANCE_STATE_READING:
                    m_inputBuffer.TrimLastChunk(dwBytesTransfered);
                    if(!more_data(hRes))
                    {
                        message_completed();
                    }
                    break;
                }
            }

            // read/write (again)
            switch(m_eState)
            {
                case INSTANCE_STATE_READING:
                {
                    while(true)
                    {
                        HRESULT hRes;
                        DWORD dwBytesTransfered, dwBytesLeftInThisMsg;

                        hRes = m_pipeInstance.PeekNamedPipe(dwBytesLeftInThisMsg);
                        if(SUCCEEDED(hRes))
                        {
							size_t nMsgLength = m_inputBuffer.GetCount() + dwBytesLeftInThisMsg;
							bool fMsgTooLong = nMsgLength >= MAX_INPUT_MESSAGE_SIZE;
							Buffer& buffer = prepare_input_buffer(fMsgTooLong, dwBytesLeftInThisMsg);
                            hRes = m_pipeInstance.ReadToArray(buffer, &m_oOverlap);
                            if(read_succeeded(hRes))
                            {
                                hRes = m_pipeInstance.GetOverlappedResult(&m_oOverlap, dwBytesTransfered, true);
                                if(read_succeeded(hRes))
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
                            else if(win32_error<ERROR_IO_PENDING>(hRes))
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

                    break;
                }
            }
        }

    protected:

        OVERLAPPED m_oOverlap;
        CHandle m_evOverlap;
        INSTANCENO m_instanceNo;
        CNamedPipe m_pipeInstance;
        CChunkedBuffer m_inputBuffer;
        CChunkedBuffer m_outputBuffer;
        INSTANCE_STATE m_eState;
        bool m_fPendingIO;
        INotify& m_notifier;

    protected:

        void message_completed()
        {
            Buffer buffer;
            m_inputBuffer.GetData(buffer);
            m_notifier.OnMessage(m_instanceNo, buffer);
            m_inputBuffer.Clear();
        }

	private:

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
    };

private:

    bool m_fValid;
    CString m_sFullPipeName;
    INSTANCENO m_instanceCnt;
    CHandle m_evWrite;
    HANDLE m_aWaitObjects[INSTANCES + 2];
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
