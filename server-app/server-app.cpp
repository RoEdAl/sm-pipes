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
#include <critical-section.hpp>
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
            _tprintf(_T("%I64i: connect\n"), instanceNo);
        }

        virtual void OnMessage(pipe_server_basics::INSTANCENO instanceNo, const pipe_server_basics::Buffer& buffer)
        {
            _tprintf(_T("%I64i: message size=%I64i\n"), instanceNo, buffer.GetCount());
        }

        virtual void OnDisconnect(pipe_server_basics::INSTANCENO instanceNo)
        {
            _tprintf(_T("%I64i: disconnect\n"), instanceNo);
        }
    };
}

int main()
{
    ServerMessages serverMessages;
#ifdef USE_LOGON_SESSION
    default_security_policy security_policy;
    named_pipe_server<2, BUFSIZE, default_security_policy> server(security_policy, serverMessages);
#else
    named_pipe_server<2, BUFSIZE, default_security_policy> server(serverMessages);
#endif

    if(!server.IsValid()) return 1;
    server.Run();
	_gettch();
    server.Stop();
    return 0;
}
