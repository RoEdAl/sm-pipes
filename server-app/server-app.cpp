//
// server-app.cpp
//

#include <targetver.h>
#include <SMPipesConfig.h>

#include <stdio.h>
#include <conio.h>
#include <tchar.h>

#include <atl-headers.h>

namespace
{
#include <named-pipe-server.hpp>
#include <security-policy.hpp>

#ifdef USE_LOGON_SESSION
    typedef logon_sesssion_security_policy default_security_policy;
#else
    typedef no_security_policy default_security_policy;
#endif

    constexpr size_t BUFSIZE = 762;

    class ServerMessages :public pipe_server_basics::INotify
    {
    public:
        virtual void OnConnect(pipe_server_basics::INSTANCENO instanceNo)
        {
            _tprintf(_T("msg(%I64i): connect\n"), instanceNo);
        }

        virtual void OnMessage(pipe_server_basics::INSTANCENO instanceNo, const pipe_server_basics::Buffer& buffer)
        {
            _tprintf(_T("msg(%I64i): message size=%I64i\n"), instanceNo, buffer.GetCount());
			if (m_pServer != nullptr)
			{
				m_pServer->SendMessage(instanceNo, buffer);
			}
        }

        virtual void OnDisconnect(pipe_server_basics::INSTANCENO instanceNo)
        {
            _tprintf(_T("msg(%I64i): disconnect\n"), instanceNo);
        }

	protected:

		named_pipe_server<2, BUFSIZE, default_security_policy>* m_pServer;

	public:

		ServerMessages()
			:m_pServer(nullptr)
		{}

		void SetServer(named_pipe_server<2, BUFSIZE, default_security_policy>& server)
		{
			m_pServer = &server;
		}
    };
}

int main()
{
	_tprintf(_T("server: version=%s\n"), _T(SMPIPES_VERSION_STR) );
	
    ServerMessages serverMessages;
#ifdef USE_LOGON_SESSION
    default_security_policy security_policy;
    named_pipe_server<2, BUFSIZE, default_security_policy> server(security_policy, serverMessages);
#else
    named_pipe_server<2, BUFSIZE, default_security_policy> server(serverMessages);
#endif

	if (!server.IsValid())
	{
		_tprintf(_T("server: invalid\n"));
		return 1;
	}

	_tprintf(_T("server: pipe=%s\n"), (LPCTSTR)server.GetPipeName() );
	serverMessages.SetServer(server);
    server.Run();

	_gettch();
	_tprintf(_T("server: stop\n"));
    server.Stop();

	_gettch();
	_tprintf(_T("server: bye\n"));
    return 0;
}
