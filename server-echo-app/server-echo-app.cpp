//
// server-echo-app.cpp
//

#include <targetver.h>
#include <SMPipesConfig.h>

#include <stdio.h>
#include <conio.h>
#include <tchar.h>

#include <atl-headers.h>
#include <rapidjson-headers.hpp>

using namespace rapidjson;

namespace
{
#include <named-pipe-server.hpp>
#include <security-policy.hpp>
#include <named-pipe-defaults.hpp>

	template<size_t S, typename... T>
	void Printf(const TCHAR(&fmt)[S], T... args)
	{
		_fputts(_T("echo-server "), stdout);
		_ftprintf(stdout, fmt, args...);
	}

#ifdef USE_LOGON_SESSION
    typedef logon_sesssion_security_policy default_security_policy;
#else
    typedef no_security_policy default_security_policy;
#endif

    class ServerMessages :public pipe_server_basics::INotify
    {
    public:
        virtual void OnConnect(pipe_server_basics::INSTANCENO instanceNo)
        {
			Printf(_T(": msg(%I64i) - connect\n"), instanceNo);
        }

        virtual void OnMessage(pipe_server_basics::INSTANCENO instanceNo, const pipe_server_basics::Buffer& buffer)
        {
			Printf(_T("< msg(%I64i) - size=%zi\n"), instanceNo, buffer.GetCount());
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
					Printf(_T("> echo(%I64i) - size=%zi\n"), instanceNo, buf_reply.GetCount());
				}
				else
				{
					Printf(_T("! msg(%I64i) - could not parse message\n"), instanceNo);
					m_pServer->SendMessage(instanceNo, buffer);
				}
			}
        }

        virtual void OnDisconnect(pipe_server_basics::INSTANCENO instanceNo, bool fOrigin)
        {
			Printf(_T(": msg(%I64i) - disconnect(%s)\n"), instanceNo, fOrigin? _T("origin") : _T("answer"));
        }

	protected:

		named_pipe_server<named_pipes_defaults::INSTANCES, named_pipes_defaults::BUFFER_SIZE, default_security_policy>* m_pServer;

	public:

		ServerMessages()
			:m_pServer(nullptr)
		{}

		void SetServer(named_pipe_server<named_pipes_defaults::INSTANCES, named_pipes_defaults::BUFFER_SIZE, default_security_policy>& server)
		{
			m_pServer = &server;
		}
    };
}

int main()
{
	Printf(_T(": version=%s\n"), _T(SMPIPES_VERSION_STR) );
	
    ServerMessages serverMessages;
#ifdef USE_LOGON_SESSION
    default_security_policy security_policy;
    named_pipe_server<named_pipes_defaults::INSTANCES, named_pipes_defaults::BUFFER_SIZE, default_security_policy> server(security_policy, serverMessages);
#else
    named_pipe_server<2, BUFSIZE, default_security_policy> server(serverMessages);
#endif

	if (!server.IsValid())
	{
		Printf(_T("! invalid - could not initialize\n"));
		return 1;
	}

	Printf(_T(": pipe=%s\n"), (LPCTSTR)server.GetPipeName() );

	serverMessages.SetServer(server);
    HRESULT hRes = server.Run();
	Printf(_T(": run hr=%08x\n"), hRes);

	_gettch();
	Printf(_T(": stop\n"));
    hRes = server.Stop();

	_gettch();
	Printf(_T(": bye hr=%08x\n"), hRes);
    return hRes;
}
