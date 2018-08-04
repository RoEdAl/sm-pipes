/*
    named-pipe-server.hpp
*/

#ifndef _NAMED_PIPE_SEVER_HPP_
#define _NAMED_PIPE_SEVER_HPP_

#include "sm-pipe-name-routines.hpp"
#include "hres-routines.hpp"

//using namespace hres_routines;

template<size_t INSTANCES, size_t BUFSIZE, typename S>
class named_pipe_server :public pipe_server_basics
{
public:

    named_pipe_server(S& securityPolicy)
    {
        m_bValid = true;

        bool bCustomSecurityAttr = false;
        CSecurityDesc sd;
        CSecurityAttributes sa;

        if(!securityPolicy.Init())
        {
            m_bValid = false;
            return;
        }

        m_instanceCnt = 0;

        m_sFullPipeName = sm_pipe_name_routines::get_full_pipe_name(securityPolicy.GetPipeName());

        if(securityPolicy.BuildSecurityDesc(sd))
        {
            sa.Set(sd);
            bCustomSecurityAttr = true;
        }

        create_event(m_evStop);
        create_event(m_evWrite);

        m_aWaitObjects[INSTANCES] = m_evStop;
        m_aWaitObjects[INSTANCES + 1] = m_evWrite;

        m_aInstance.SetCount(INSTANCES);

        for(size_t i = 0; i < INSTANCES; ++i)
        {
            CHandle ev;
            CNamedPipe pipe;

            create_event(ev);
            create_named_pipe(pipe, bCustomSecurityAttr, sa);

            m_aWaitObjects[i] = ev.m_h;

            CPipeInstance* pPipeInstance = new CPipeInstance(++m_instanceCnt, pipe, ev);
            m_aInstance.GetAt(i).Attach(pPipeInstance);
        }
    }

    // run in separate thread
    HRESULT Run()
    {
        if(m_thread.m_h != nullptr) return E_FAIL;

        HANDLE hThread = ::CreateThread(nullptr, 0, thread_proc, this, CREATE_SUSPENDED, nullptr);
        if(hThread == nullptr)
        {
            return AtlHresultFromLastError();
        }

        m_thread.Attach(hThread);
        ::ResumeThread(hThread);
        return S_OK;
    }

    HRESULT Stop()
    {
        if(m_thread.m_h == nullptr) return S_FALSE;

        set_event(m_evStop);
        DWORD dwRes = ::WaitForSingleObject(m_thread, WORKER_THREAD_FINISH_TIMEOUT);
        if(dwRes == WAIT_TIMEOUT)
        {
            ::TerminateThread(m_thread, TERMINATED_THREAD_EXIT_CODE);
            return S_FALSE;
        }

        m_thread.Close();
        return S_OK;
    }

    ~named_pipe_server()
    {

    }

    bool IsValid() const
    {
        return m_bValid;
    }

private:

    bool m_bValid;
    bool m_bCustomSecurityDesc;
    CString m_sFullPipeName;

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

    void connect_pipes()
    {
        for(size_t i = 0; i < INSTANCES; ++i)
        {
            m_aInstance[i]->ConnectToNewClient();
        }
    }

    void disconnect_pipes()
    {
        HANDLE aHandle[INSTANCES], h;
        DWORD dwCnt = 0, dwWaitRes;
        HRESULT hRes;

        for(size_t i = 0; i < INSTANCES; ++i)
        {
            h = m_aInstance[i]->CancelPendingOperation();
            if(h != nullptr)
            {
                aHandle[dwCnt++] = h;
            }
        }

        dwWaitRes = ::WaitForMultipleObjects(dwCnt, aHandle, true, INFINITE);
        ATLASSERT(dwWaitRes >= WAIT_OBJECT_0 && dwWaitRes < (WAIT_OBJECT_0 + dwCnt));

        for(size_t i = 0; i < INSTANCES; ++i)
        {
            hRes = m_aInstance[i]->FinishCancelingAndDisconnect();
        }
    }

    class CPipeInstance
    {
    public:

        CPipeInstance(INSTANCENO instanceNo, CNamedPipe& pipeInstance, CHandle& evOverlap)
            :m_instanceNo(instanceNo), m_pipeInstance(pipeInstance), m_evOverlap(evOverlap),
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
                case HRESULT_FROM_WIN32(ERROR_IO_PENDING):
                m_fPendingIO = true;
                break;

                case HRESULT_FROM_WIN32(ERROR_PIPE_CONNECTED):
                set_event(m_evOverlap);
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

            HRESULT hRes;

            if(m_fPendingIO)
            {
                DWORD dwBytesTransfered;
                hRes = m_pipeInstance.GetOverlappedResult(&m_oOverlap, dwBytesTransfered, true);
                if(abort_succeeded(hRes))
                {
                    m_fPendingIO = false;
                }
            }
            hRes = m_pipeInstance.DisconnectNamedPipe();
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
                        DWORD dwBytesTransfered, dwBytesLeftInThisMessage;

                        hRes = m_pipeInstance.PeekNamedPipe(dwBytesLeftInThisMessage);
                        if(SUCCEEDED(hRes))
                        {
                            // TODO: validate message size
                            Buffer& buffer = m_inputBuffer.AddBuffer(dwBytesLeftInThisMessage > 0 ? dwBytesLeftInThisMessage : BUFSIZE);
                            hRes = m_pipeInstance.ReadToArray(buffer, &m_oOverlap);
                            if(read_succeeded(hRes))
                            {
                                hRes = m_pipeInstance.GetOverlappedResult(&m_oOverlap, dwBytesTransfered, true);
                                if(read_succeeded(hRes))
                                {
                                    m_inputBuffer.TrimLastChunk(dwBytesTransfered);
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

    protected:

        void message_completed()
        {
            Buffer buffer;
            m_inputBuffer.GetData(buffer);
            // TODO: do something with this buffer
            m_inputBuffer.Clear();
        }
    };

    INSTANCENO m_instanceCnt;
    CHandle m_evStop;
    CHandle m_evWrite;
    HANDLE m_aWaitObjects[INSTANCES + 2];
    CAutoPtrArray<CPipeInstance> m_aInstance;
    CHandle m_thread;

protected:


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

    static void create_event(CHandle& ev)
    {
        HANDLE hEvent = ::CreateEvent(
            nullptr,    // default security attribute 
            true,    // manual-reset event 
            false,    // initial state = nonsignaled 
            nullptr);   // unnamed event object 

        if(hEvent == nullptr)
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
