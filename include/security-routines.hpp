/*
    security_routines.hpp
*/

#ifndef _SECURITY_ROUTINES_HPP_
#define _SECURITY_ROUTINES_HPP_

namespace security_routines
{
	bool get_logon_sid(CSid* pLogonSid)
	{
		CAccessToken atoken;
		if (!atoken.GetEffectiveToken(TOKEN_QUERY))
		{
			return false;
		}
		return atoken.GetLogonSid(pLogonSid);
	}

	bool get_owner_and_logon_sid(CSid* pOwnerSid, CSid* pLogonSid)
	{
		CAccessToken atoken;
		if (!atoken.GetEffectiveToken(TOKEN_QUERY))
		{
			return false;
		}
		return atoken.GetLogonSid(pLogonSid) && atoken.GetOwner(pOwnerSid);
	}
}

#endif // _SECURITY_ROUTINES_HPP_
