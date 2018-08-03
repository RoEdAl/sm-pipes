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
#include <sm-pipe-name-routines.hpp>
#include <pipe-server-basics.hpp>
#include <named-pipe-server.hpp>
}

int main()
{
    named_pipe_server<2, 4 * 1024> server;
    if(!server.IsValid()) return 1;
    server.Run();
    _getch();
    server.Stop();
    return 0;
}
