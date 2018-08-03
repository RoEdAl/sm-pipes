//
// server-app.cpp
//

#include <targetver.h>
#include <SMPipesConfig.h>

#include <stdio.h>
#include <tchar.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

#include <aclapi.h>
#include <sddl.h>

#include <atlsecurity.h>
#include <atlstr.h>
#include <atlfile.h>
#include <atlcoll.h>

namespace
{
#include <sm-pipe-name-routines.hpp>
#include <pipe-server-basics.hpp>
#include <named-pipe-server.hpp>
}

int main()
{
	named_pipe_server<2,4*1024> server;
	if (!server.IsValid()) return 1;
	server.Run();
    return 0;
}
