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
#include <pipe-server-basics.hpp>
#include <named-pipe-server.hpp>
#include <security-policy.hpp>

#ifdef USE_LOGON_SESSION
    typedef logon_sesssion_security_policy default_security_policy;
#else
    typedef no_security_policy default_security_policy;
#endif

    constexpr size_t BUFSIZE = 762;
}

int main()
{
#ifdef USE_LOGON_SESSION
    default_security_policy security_policy;
    named_pipe_server<2, BUFSIZE, default_security_policy> server(security_policy);
#else
    named_pipe_server<2, BUFSIZE, default_security_policy> server;
#endif

    if(!server.IsValid()) return 1;
    server.Run();
    _getch();
    server.Stop();
    return 0;
}
