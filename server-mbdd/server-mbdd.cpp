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

using namespace rapidjson;

namespace
{
#include <named-pipe-server.hpp>
#include <security-policy.hpp>

#ifdef USE_LOGON_SESSION
    typedef logon_sesssion_security_policy default_security_policy;
#else
    typedef no_security_policy default_security_policy;
#endif

	constexpr size_t INSTANCES = 2;
    constexpr size_t BUFSIZE = 762;

    class ServerMessages :public pipe_server_basics::INotify
    {
    public:
        virtual void OnConnect(pipe_server_basics::INSTANCENO instanceNo)
        {
        }

        virtual void OnMessage(pipe_server_basics::INSTANCENO instanceNo, const pipe_server_basics::Buffer& buffer)
        {
			if (m_pServer != nullptr)
			{
				Document msg;
				ParseResult res = msg.Parse(reinterpret_cast<const char*>(buffer.GetData()), buffer.GetCount());
				if (res)
				{
					Document reply;
					reply.SetObject();
					Document::AllocatorType& alloc = msg.GetAllocator();
					reply.AddMember("echo", msg, alloc);

					StringBuffer strm;
					Writer<StringBuffer> writer(strm);
					reply.Accept(writer);

					pipe_server_basics::Buffer buf_reply;
					buf_reply.SetCount(strm.GetSize());
					Checked::memcpy_s(buf_reply.GetData(), buf_reply.GetCount(), strm.GetString(), strm.GetSize());

					m_pServer->SendMessage(instanceNo, buf_reply);
				}
				else
				{
					m_pServer->SendMessage(instanceNo, buffer);
				}
			}
        }

        virtual void OnDisconnect(pipe_server_basics::INSTANCENO instanceNo)
        {
        }

	protected:

		named_pipe_server<INSTANCES, BUFSIZE, default_security_policy>* m_pServer;

	public:

		ServerMessages()
			:m_pServer(nullptr)
		{}

		void SetServer(named_pipe_server<INSTANCES, BUFSIZE, default_security_policy>& server)
		{
			m_pServer = &server;
		}
    };
}

extern "C" int __stdcall SMSrvApiLevel(void)
{
	return SM_SRV_API_LEVEL_1;
}

extern "C" void* __stdcall SMSrvRegister(SM_SRV_HANDLER handler)
{
	CStringW ala(L"Kiszonka ma pozimicę");
	handler(SM_SRV_CMD_URL, const_cast<LPWSTR>((LPCWSTR)ala));
	return handler;
}

extern "C" int __stdcall SMSrvUnRegister(void* handle)
{
	return S_OK;
}
