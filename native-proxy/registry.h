/*
    registry.h
*/

#ifndef _REGISTRY_H_
#define _REGiSTRY_H_

HRESULT register_chrome_native_msg_host(bool, bool, LPCTSTR);
HRESULT unregister_chrome_native_msg_host(bool, bool);

#endif // _REGiSTRY_H_