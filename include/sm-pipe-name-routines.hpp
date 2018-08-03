/*
    sm-pipe-name-routines.hpp
*/

#ifndef _SM_PIPE_NAME_ROUTINES_HPP_
#define _SM_PIPE_NAME_ROUTINES_HPP_

namespace sm_pipe_name_routines
{
    const TCHAR NAMED_PIPE_PREFIX[] = _T("\\\\.\\pipe\\");
    const TCHAR SM_PIPE_PREFIX[] = _T("SuperMemo");

    /*
        SID: S-1-5-5-X-Y
        Name: Logon Session
        Description: A logon session. The X and Y values for these SIDs are different for each session.
        Pipe name: SuperMemo-X-Y
    */
    CString get_sm_pipe_name(const CSid& logon_sid)
    {
        CString res(logon_sid.Sid());
        res.Delete(0, 7);
        res.Insert(0, SM_PIPE_PREFIX);
        return res;
    }

    CString get_full_pipe_name(const CString& sName)
    {
        CString res(NAMED_PIPE_PREFIX);
        res += sName;
        return res;
    }
};

#endif // _SM_PIPE_NAME_ROUTINES_HPP_
