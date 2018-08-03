/*
    named-pipe-server.hpp
*/

#ifndef _NAMED_PIPE_SEVER_HPP_
#define _NAMED_PIPE_SEVER_HPP_

#include "security-routines.hpp"

template<size_t INSTANCES, size_t BUFSIZE>
class named_pipe_server :public pipe_server_basics
{
public:

    named_pipe_server()
    {
        if(!security_routines::get_owner_and_logon_sid(&m_ownerSid, &m_logonSid))
        {
            return;
        }

        m_instanceCnt = 0;

        m_sFullPipeName = sm_pipe_name_routines::get_full_pipe_name(sm_pipe_name_routines::get_sm_pipe_name(m_logonSid));

        CSecurityDesc sd;
        build_security_descriptor(sd);
        m_sa.Set(sd);

        create_event(m_evStop);
        create_event(m_evWrite);

        m_aWaitObjects[INSTANCES] = m_evStop;
        m_aWaitObjects[INSTANCES + 1] = m_evWrite;

        m_aInstanceEvent.SetCount(INSTANCES);
        m_aInstance.SetCount(INSTANCES);

        for(size_t i = 0; i < INSTANCES; ++i)
        {
            CHandle ev;
            CNamedPipe pipe;

            create_event(ev);
            create_named_pipe(pipe);

            m_aWaitObjects[i] = ev.m_h;

            CPipeInstance* pPipeInstance = new CPipeInstance(++m_instanceCnt, pipe, ev.m_h);
            pPipeInstance->ConnectToNewClient();
            m_aInstance.GetAt(i).Attach(pPipeInstance);

            m_aInstanceEvent.GetAt(i).Attach(ev.Detach());

        }
    }

    // run in separate thread
    HRESULT Run()
    {
        if(m_thread.m_h != NULL) return E_FAIL;

        DWORD idThread;

        HANDLE hThread = ::CreateThread(NULL, 0, thread_proc, this, CREATE_SUSPENDED, &idThread);
        if(hThread == NULL)
        {
            return AtlHresultFromLastError();
        }

        m_thread.Attach(hThread);
        ::ResumeThread(hThread);
        return S_OK;
    }

    HRESULT Stop()
    {
        if(m_thread.m_h == NULL) return S_FALSE;

        set_event(m_evStop);
        DWORD dwRes = ::WaitForSingleObject(m_thread, WORKER_THREAD_FINISH_TIMEOUT);
        if(dwRes == WAIT_TIMEOUT)
        {
            ::TerminateThread(m_thread, TERMINATED_THREAD_EXIT_CODE);
            return S_FALSE;
        }

        return S_OK;
    }

    ~named_pipe_server()
    {

    }

    bool IsValid() const
    {
        return m_logonSid.IsValid();
    }

private:

    CSid m_ownerSid;
    CSid m_logonSid;
    CString m_sFullPipeName;
    CSecurityAttributes m_sa;

private:

    static DWORD WINAPI thread_proc(LPVOID _pThis)
    {
        named_pipe_server* pThis = static_cast<named_pipe_server*>(_pThis);
        _ATLTRY
        {
            pThis->thread_proc();
            return S_OK;
        }
            _ATLCATCH(e)
        {
            return e.m_hr;
        }
    };

    void thread_proc()
    {
        ATLASSERT(IsValid());

        bool bStop = false;

        while(!bStop)
        {
            DWORD dwWait = ::WaitForMultipleObjects(INSTANCES + 2, m_aWaitObjects, false, PIPE_WAIT_TIMEOUT);

            if(dwWait == WAIT_TIMEOUT) continue;
            if(dwWait == WAIT_FAILED)
            {
                bStop = true;
                AtlThrowLastWin32();
                continue;
            }

            int i = dwWait - WAIT_OBJECT_0;
            if((i < 0) || (i >= (INSTANCES + 2)))
            {
                // index out of range
                bStop = true;
            }

            switch(i)
            {
                case INSTANCES: // m_evStop
                bStop = true;
                break;

                case INSTANCES + 1: // m_evWrite
                // TODO
                break;

                default:
                {
                    CAutoPtr<CPipeInstance>& pPipe = m_aInstance[i];
                    _ATLTRY
                    {
                        pPipe->Run();
                    }
                        _ATLCATCH(e)
                    {
                        if(e.m_hr == HRESULT_FROM_WIN32(ERROR_BROKEN_PIPE))
                        {
                            pPipe->DisconnectAndReconnect(++m_instanceCnt);
                        }
                    }
                    break;
                }
            }
        }

        cancel_pending_operations();
    }

    void cancel_pending_operations()
    {
        for(size_t i = 0; i < INSTANCES; ++i)
        {
            m_aInstance[i]->CancelPendingOperation();
        }
    }

    class CPipeInstance
    {
    public:

        CPipeInstance(INSTANCENO _instanceNo, CNamedPipe& _pipeInstance, HANDLE _hEvent)
        {
            instanceNo = _instanceNo;

            ZeroMemory(&oOverlap, sizeof(OVERLAPPED));
            oOverlap.hEvent = _hEvent;

            pipeInstance.Attach(_pipeInstance.Detach());

            eState = INSTANCE_STATE_CONNECTING;
            fPendingIO = false;
        }

        void ConnectToNewClient()
        {
            HRESULT hRes = pipeInstance.ConnectNamedPipe(&oOverlap);

            switch(hRes)
            {
                case HRESULT_FROM_WIN32(ERROR_IO_PENDING):
                fPendingIO = true;
                break;

                case HRESULT_FROM_WIN32(ERROR_PIPE_CONNECTED):
                if(!::SetEvent(oOverlap.hEvent))
                {
                    AtlThrowLastWin32();
                }
                break;

                default:
                AtlThrow(hRes);
                break;
            }

            eState = fPendingIO ? INSTANCE_STATE_CONNECTING : INSTANCE_STATE_READING;
        }

        void DisconnectAndReconnect(INSTANCENO _instanceNo)
        {
            ATLASSERT(!fPendingIO);
            HRESULT hRes = pipeInstance.DisconnectNamedPipe();
            ATLASSUME(SUCCEEDED(hRes));

            eState = INSTANCE_STATE_CONNECTING;
            fPendingIO = false;
            instanceNo = _instanceNo;
            inputBuffer.Clear();
            outputBuffer.Clear();
            ConnectToNewClient();
        }

        void Run()
        {
            DWORD cbTransfered;
            fPendingIO = false;
            HRESULT hRes = pipeInstance.GetOverlappedResult(&oOverlap, cbTransfered, false);
            if(!read_succeeded(hRes))
            {
                AtlThrow(hRes);
            }

            switch(eState)
            {
                case INSTANCE_STATE_CONNECTING:
                eState = INSTANCE_STATE_READING;
                break;

                case INSTANCE_STATE_READING:
                inputBuffer.TrimLastChunk(cbTransfered);
                if(!more_data(hRes))
                {
                    message_completed();
                }
                break;
            }

            // read/write (again)
            switch(eState)
            {
                case INSTANCE_STATE_READING:
                {
                    while(true)
                    {
                        DWORD cbBytesLeftInThisMessage;
                        hRes = pipeInstance.PeekNamedPipe(cbBytesLeftInThisMessage);
                        if(SUCCEEDED(hRes))
                        {
                            // TODO: validate message size
                            Buffer& buffer = inputBuffer.AddBuffer(cbBytesLeftInThisMessage > 0 ? cbBytesLeftInThisMessage : BUFSIZE);
                            hRes = pipeInstance.Read(buffer.GetData(), static_cast<DWORD>(buffer.GetCount()), &oOverlap);
                            if(read_succeeded(hRes))
                            {
                                hRes = pipeInstance.GetOverlappedResult(&oOverlap, cbTransfered, false);
                                if(read_succeeded(hRes))
                                {
                                    inputBuffer.TrimLastChunk(cbTransfered);
                                    if(!more_data(hRes))
                                    {
                                        message_completed();
                                    }
                                }
                                else
                                {
                                    AtlThrow(hRes);
                                }
                            }
                            else if(hRes != HRESULT_FROM_WIN32(ERROR_IO_PENDING))
                            {
                                AtlThrow(hRes);
                            }
                            else
                            {
                                fPendingIO = true;
                                break;
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

        void CancelPendingOperation()
        {
            if(!fPendingIO) return;

            HRESULT hRes = pipeInstance.CancelIo();
            hRes = pipeInstance.DisconnectNamedPipe();
        }

    protected:

        OVERLAPPED oOverlap;
        INSTANCENO instanceNo;
        CNamedPipe pipeInstance;
        CChunkedBuffer inputBuffer;
        CChunkedBuffer outputBuffer;
        INSTANCE_STATE eState;
        bool fPendingIO;

    protected:

        void message_completed()
        {
            Buffer buffer;
            inputBuffer.GetData(buffer);
            // TODO: do something with this buffer
            inputBuffer.Clear();
        }
    };

    INSTANCENO m_instanceCnt;
    CHandle m_evStop;
    CHandle m_evWrite;
    HANDLE m_aWaitObjects[INSTANCES + 2];
    CAtlArray<CHandle> m_aInstanceEvent;
    CAutoPtrArray<CPipeInstance> m_aInstance;
    CHandle m_thread;

protected:

    void build_security_descriptor(CSecurityDesc& sd) const
    {
        CDacl dacl;
        dacl.AddAllowedAce(m_ownerSid, STANDARD_RIGHTS_ALL | FILE_ALL_ACCESS);
        dacl.AddAllowedAce(Sids::Admins(), FILE_ALL_ACCESS);
        dacl.AddAllowedAce(m_logonSid, FILE_GENERIC_READ | FILE_WRITE_DATA);
        sd.SetDacl(dacl);
    }

    void create_named_pipe(CNamedPipe& pipe) const
    {
        HRESULT hRes = pipe.CreateNamedPipe(m_sFullPipeName,
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            INSTANCES, BUFSIZE, BUFSIZE, PIPE_CONNECT_TIMEOUT,
            const_cast<CSecurityAttributes *>(&m_sa));

        if(FAILED(hRes))
        {
            AtlThrow(hRes);
        }
    }

    static void create_event(CHandle& ev)
    {
        HANDLE hEvent = ::CreateEvent(
            NULL,    // default security attribute 
            true,    // manual-reset event 
            false,    // initial state = signaled 
            NULL);   // unnamed event object 

        if(hEvent == NULL)
        {
            AtlThrowLastWin32();
        }

        ev.Attach(hEvent);
    }

    static void set_event(CHandle& ev)
    {
        BOOL fResult = ::SetEvent(ev);
        if(!fResult)
        {
            AtlThrowLastWin32();
        }
    }
};

#endif // _NAMED_PIPE_SEVER_HPP_
