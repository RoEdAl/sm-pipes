//
// client-app.cpp
//

#include <targetver.h>
#include <SMPipesConfig.h>

#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <atl-headers.h>

namespace
{
#include <named-pipe-client.hpp>
#include <hres-routines.hpp>
#include <named-pipe.hpp>
#include <security-policy.hpp>

#ifdef USE_LOGON_SESSION
    typedef logon_sesssion_security_policy default_security_policy;
#else
    typedef no_security_policy default_security_policy;
#endif
}

using namespace hres_routines;

int main()
{
#ifdef USE_LOGON_SESSION
    default_security_policy security_policy;
    if(!security_policy.Init()) return 1;
    CString sPipeName(sm_pipe_name_routines::get_full_pipe_name(security_policy.GetPipeName()));
#else
    CString sPipeName(sm_pipe_name_routines::get_full_pipe_name(default_security_policy::GetPipeName()));
#endif

	named_pipe_client<512> pipe_client(sPipeName);

	pipe_client.Run();

	CAtlArray<BYTE> buffer;
	buffer.SetCount(5 * 1024);
	FillMemory(buffer.GetData(), buffer.GetCount(), 0x54);

	// 1
	pipe_client.SendMessage(buffer);
	_gettch();

	// 2
	buffer.SetCount(buffer.GetCount() / 2);
	pipe_client.SendMessage(buffer);

	_gettch();
	pipe_client.Stop();

	_gettch();
    return 0;
}

