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

    CNamedPipe pipe;

    while(true)
    {
        HRESULT hRes = pipe.Create(sPipeName, GENERIC_READ | GENERIC_WRITE, 0, OPEN_EXISTING);
        if(win32_error<ERROR_PIPE_BUSY>(hRes))
        {
            if(!::WaitNamedPipe(sPipeName, 5000))
            {
                return 2;
            }
            else
            {
                continue;
            }
        }
        break;
    }

    HRESULT hRes = pipe.SetNamedPipeHandleState(PIPE_READMODE_MESSAGE);
    if(FAILED(hRes))
    {
        _tprintf(TEXT("SetNamedPipeHandleState failed. GLE=%d\n"), hRes);
        return -1;
    }

    CAtlArray<BYTE> buffer;
    buffer.SetCount(5 * 1024);
    FillMemory(buffer.GetData(), buffer.GetCount(), 0x54);

    // 1
    BOOL fWriteStatus = pipe.Write(buffer.GetData(), static_cast<DWORD>(buffer.GetCount()));
	_gettch();

    // 2
    fWriteStatus = pipe.Write(buffer.GetData(), static_cast<DWORD>(buffer.GetCount()/2));
	_gettch();
    return 0;
}

