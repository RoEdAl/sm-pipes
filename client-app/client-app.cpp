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
#include <security-routines.hpp>
#include <sm-pipe-name-routines.hpp>
#include <named-pipe.hpp>
}

int main()
{
    CSid logonSid;

    if(!security_routines::get_logon_sid(&logonSid)) return 1;
    CString sPipeName(sm_pipe_name_routines::get_full_pipe_name(sm_pipe_name_routines::get_sm_pipe_name(logonSid)));
    CNamedPipe pipe;

    while(true)
    {
        HRESULT hRes = pipe.Create(sPipeName, GENERIC_READ | GENERIC_WRITE, 0, OPEN_EXISTING);
        if(hRes == HRESULT_FROM_WIN32(ERROR_PIPE_BUSY))
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
    _getch();

    // 2
    fWriteStatus = pipe.Write(buffer.GetData(), static_cast<DWORD>(buffer.GetCount()/2));
    _getch();
    return 0;
}

