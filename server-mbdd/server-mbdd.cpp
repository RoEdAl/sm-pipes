﻿//
// server-mbdd.cpp
//

#include <targetver.h>
#include <SMPipesConfig.h>

#include <atl-headers.h>
#include <rapidjson-headers.hpp>
#include <sm-srv.h>

namespace
{
#include <named-pipe-server.hpp>
#include <security-policy.hpp>
#include <named-pipe-defaults.hpp>
#include <msg-process.hpp>

#ifdef USE_LOGON_SESSION
    typedef logon_sesssion_security_policy default_security_policy;
#else
    typedef no_security_policy default_security_policy;
#endif

    typedef named_pipe_server<named_pipes_defaults::INSTANCES, named_pipes_defaults::BUFFER_SIZE, default_security_policy> np_server;

    class svr_instance
    {
        public:

#ifdef USE_LOGON_SESSION
            svr_instance(SM_SRV_HANDLER svr_handler)
                :m_receiver(m_server, svr_handler),
                 m_server(m_security_policy, m_receiver)
            {}
#else
            svr_instance(SM_SRV_HANDLER svr_handler)
                : m_receiver(m_server, svr_handler),
                  m_server(m_receiver),
            {}
#endif

            HRESULT Run()
            {
                return m_server.Run();
            }

            HRESULT Stop()
            {
                return m_server.Stop();
            }

        protected:

            class receiver :public pipe_server_basics::INotify
            {
            public:
                virtual void OnConnect(pipe_server_basics::INSTANCENO instanceNo)
                {}

                virtual void OnMessage(pipe_server_basics::INSTANCENO instanceNo, const pipe_server_basics::Buffer& buffer)
                {
                    msg_process::process_message(m_server, instanceNo, m_srv_handler, buffer);
                }

                virtual void OnDisconnect(pipe_server_basics::INSTANCENO instanceNo)
                {}

            protected:

                np_server& m_server;
                SM_SRV_HANDLER m_srv_handler;

            public:

                receiver(np_server& server, SM_SRV_HANDLER srv_handler)
                    :m_server(server), m_srv_handler(srv_handler)
                {}
            };

    protected:

#ifdef USE_LOGON_SESSION
        default_security_policy m_security_policy;
#endif
        receiver m_receiver;
        np_server m_server;
    };
}

extern "C" int __stdcall SMSrvApiLevel(void)
{
	return SM_SRV_API_LEVEL_1;
}

extern "C" void* __stdcall SMSrvRegister(SM_SRV_HANDLER handler)
{
    svr_instance* pInstance = nullptr;
    
    _ATLTRY
    {
        pInstance = new svr_instance(handler);
        HRESULT hRes = pInstance->Run();

        if(FAILED(hRes))
        {
            delete pInstance;
            return nullptr;
        }

        return reinterpret_cast<void*>(pInstance);
    }
    _ATLCATCH(e)
    {
        HRESULT hRes = e;
        if(pInstance != nullptr)
        {
            delete pInstance;
        }
        return nullptr;
    }
    _ATLCATCHALL()
    {
        if(pInstance != nullptr)
        {
            delete pInstance;
        }
        return nullptr;
    }
}

extern "C" int __stdcall SMSrvUnRegister(void* handle)
{
    if(handle == nullptr)
    {
        return S_OK;
    }

    _ATLTRY
    {
        svr_instance* pInstance = reinterpret_cast<svr_instance*>(handle);
        HRESULT hRes = pInstance->Stop();
        delete pInstance;
        return hRes;
    }
    _ATLCATCH(e)
    {
        return e;
    }
    _ATLCATCHALL()
    {
        return E_FAIL;
    }
}
