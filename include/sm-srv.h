/*
   sm-srv.h
*/

#ifndef _SM_SRV_H_
#define _SM_SRV_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
    API levels
*/
#define SM_SRV_API_LEVEL 1 // our first API level

/*
    commands:
    
    SM_CMD_NONE		internal use only
    SM_CMD_URL		single URL
*/
#define SM_SRV_CMD_NONE 0
#define SM_SRV_CMD_URL 1

/*
    replies
    
    SM_SRV_RPLY_OK			everything's fine
    SM_SRV_RPLY_FAIL		something goes wrong
    SM_SRV_RPLY_NOREPLY		special value, do not reply
*/
#define SM_SRV_RPLY_OK 0
#define SM_SRV_RPLY_FAIL -1
#define SM_SRV_RPLY_NOREPLY -2

/*
	SM_SRV_HANDLER - user-specified handler function.
    
    cmd - command
	
		SM_SRV_CMD_...
    
    val
    
        - cmd==SM_SRV_CMD_URL - null-terminated, UTF-16 encoded URL
    
    return value:
    
    A value which will be send back to the caller.
	SM_SRV_RPLY_...

	- Zero and all positive values indicates success.
	- Negative values indicates failure.
	- Return SM_SRV_RPLY_NOREPLY if you don't want send reply.
    
    Notes:
    
    - Hadler will be called from worker thread created by SMSrvRegister function.
	- val should be considered as read-only pointer.
    - val should be considered as valid pointer during handler's call only.
    - You should avoid lengthy operations in handler function.
*/
typedef int (__stdcall *SM_SRV_HANDLER)(int cmd, void* val);

/*
    SMSrvApiLevel
    
    return value:

    Current API level.
*/
int __stdcall SMSrvApiLevel(void);

/*
    SMSrvRegister
    
    handler - address of user-specified function
    
    return value:
    
    0 (NULL)			fail to register
    any other value		successfull call, handle required to unregister
*/
void* __stdcall SMSrvRegister(SM_SRV_HANDLER handler);

/*
    SMSrvUnRegister
    
    handle - value returned by SMSrvRegister function
    
    return value:
    
    0					successfull operation
    any other value		failure

	Notes:

	- SMSrvRegister and SMSrvUnRegister should be called from the same thread.
	- handle parameter could be NULL (0)
*/
int __stdcall SMSrvUnRegister(void* handle);

#ifdef __cplusplus
}
#endif

#endif // _SM_SRV_H_
