//
// server-mbdd.cpp
//

#include <targetver.h>
#include <SMPipesConfig.h>

#include <atl-headers.h>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/reader.h>
#include <rapidjson/error/en.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>

#include <sm-srv.h>

namespace
{
    namespace json = rapidjson;

    template<typename T, size_t S, size_t BUF_SIZE = 1024>
    bool get_string_val(const json::GenericValue<T>& obj, const char(&key)[S], CString& sVal)
    {
        if(obj.HasMember(key))
        {
            const json::Value& val = obj[key];
            if(val.IsString())
            {
                if(val.GetStringLength() > 0)
                {
                    CA2WEX<BUF_SIZE> conv(val.GetString(), CP_UTF8);
                    sVal = conv;
                }
                else
                {
                    sVal.Empty();
                }

                return true;
            }
        }
        return false;
    }

    template<typename T>
    bool get_msg_cmd(const json::GenericDocument<T>& msg, CString& cmd)
    {
        return get_string_val(msg, "cmd", cmd);
    }

    template<typename T>
    bool get_msg_string_val(const json::GenericDocument<T>& msg, CString& val)
    {
        return get_string_val(msg, "val", val);
    }
}

namespace
{
#include <named-pipe-server.hpp>
#include <security-policy.hpp>
#include <named-pipe-defaults.hpp>

#ifdef USE_LOGON_SESSION
    typedef logon_sesssion_security_policy default_security_policy;
#else
    typedef no_security_policy default_security_policy;
#endif

    typedef named_pipe_server<named_pipes_defaults::INSTANCES, named_pipes_defaults::BUFFER_SIZE, default_security_policy> np_server;

    namespace json = rapidjson;

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
                {
                }

                virtual void OnMessage(pipe_server_basics::INSTANCENO instanceNo, const pipe_server_basics::Buffer& buffer)
                {
                    json::Document msg;
                    json::ParseResult res = msg.Parse(reinterpret_cast<const char*>(buffer.GetData()), buffer.GetCount());
                    if(res)
                    {
                        process_message(instanceNo, msg);
                    }
                }

                virtual void OnDisconnect(pipe_server_basics::INSTANCENO instanceNo)
                {
                }

            protected:

                np_server& m_server;
                SM_SRV_HANDLER m_svr_handler;

            public:

                receiver(np_server& server, SM_SRV_HANDLER svr_handler)
                    :m_server(server),m_svr_handler(svr_handler)
                {}

            protected:

                template<typename T>
                void process_message(pipe_server_basics::INSTANCENO instanceNo, json::GenericDocument<T>& msg)
                {
                    bool f;
                    CString sCmd;
                    int nRes = SM_SRV_RPLY_NOREPLY;

                    f = get_msg_cmd(msg, sCmd);
                    if(!f) return;

                    // SM_SRV_CMD_URL
                    if(!sCmd.CompareNoCase(_T("url")))
                    {
                        CString sUrl;
                        f = get_msg_string_val(msg, sUrl);
                        if(!f || sUrl.IsEmpty()) return;

                        nRes = m_svr_handler(SM_SRV_CMD_URL, const_cast<LPTSTR>((LPCTSTR)sUrl));
                        send_message_to_sender(instanceNo, msg, nRes);
                    }
                }

                template<typename T>
                void send_message_to_sender(pipe_server_basics::INSTANCENO instanceNo, json::GenericDocument<T>& msg, int nReply)
                {
                    if(nReply == SM_SRV_RPLY_NOREPLY)
                    {
                        return;
                    }

                    json::Document reply;
                    reply.SetObject();
                    auto& alloc = reply.GetAllocator();

                    reply.AddMember("rply", nReply, alloc);
                    reply.AddMember("cmd", msg["cmd"], alloc);
                    reply.AddMember("val", msg["val"], alloc);

                    send_message_to_sender(instanceNo, reply);
                }

                template<typename T>
                void send_message_to_sender(pipe_server_basics::INSTANCENO instanceNo, const json::GenericDocument<T>& msg)
                {
                    json::StringBuffer strm;
                    json::Writer<json::StringBuffer> writer(strm);
                    msg.Accept(writer);

                    pipe_server_basics::Buffer buf_reply;
                    size_t nBufferSize = strm.GetSize();
                    buf_reply.SetCount(nBufferSize);
                    Checked::memcpy_s(buf_reply.GetData(), nBufferSize, strm.GetString(), nBufferSize);

                    m_server.SendMessage(instanceNo, buf_reply);
                }
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
    svr_instance* pInstance = new svr_instance(handler);
    HRESULT hRes = pInstance->Run();

    if(FAILED(hRes))
    {
        delete pInstance;
        return nullptr;
    }

	return reinterpret_cast<void*>(pInstance);
}

extern "C" int __stdcall SMSrvUnRegister(void* handle)
{
    if(handle == nullptr)
    {
        return S_OK;
    }

    svr_instance* pInstance = reinterpret_cast<svr_instance*>(handle);
    HRESULT hRes = pInstance->Stop();
    delete pInstance;

	return hRes;
}
