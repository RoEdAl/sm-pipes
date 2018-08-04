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

    // specify ONE security policy
    //typedef logon_sesssion_security_policy default_security_policy;
    typedef no_security_policy default_security_policy;
}

int main()
{
    default_security_policy security_policy;
    named_pipe_server<2, 4 * 1024, default_security_policy> server(security_policy);
    if(!server.IsValid()) return 1;
    server.Run();
    _getch();
    server.Stop();
    return 0;
}
