/*
    security-policy.hpp
*/

#ifndef _SECURITY_POLICY_HPP_
#define _SECURITY_POLICY_HPP_

#include "security-routines.hpp"
#include "sm-pipe-name-routines.hpp"

struct no_security_policy
{
    static bool Init()
    {
        return true;
    }

    static LPCTSTR GetPipeName()
    {
        return sm_pipe_name_routines::SM_PIPE_PREFIX;
    }

    static bool BuildSecurityDesc(CSecurityDesc& sd)
    {
        return false;
    }
};

class logon_sesssion_security_policy
{
public:

    bool Init()
    {
        return security_routines::get_owner_and_logon_sid(&m_ownerSid, &m_logonSid);
    }

    CString GetPipeName() const
    {
        return sm_pipe_name_routines::get_sm_pipe_name(m_logonSid);
    }

    bool BuildSecurityDesc(CSecurityDesc& sd) const
    {
        CDacl dacl;
        dacl.AddAllowedAce(m_ownerSid, STANDARD_RIGHTS_ALL | FILE_ALL_ACCESS);
        dacl.AddAllowedAce(Sids::Admins(), FILE_ALL_ACCESS);
        dacl.AddAllowedAce(m_logonSid, FILE_GENERIC_READ | FILE_WRITE_DATA);
        sd.SetDacl(dacl);

        return true;
    }

protected:

    CSid m_ownerSid;
    CSid m_logonSid;

};


#endif // _SECURITY_POLICY_HPP_
