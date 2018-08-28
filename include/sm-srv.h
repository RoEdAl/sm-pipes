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
#define SM_SRV_API_LEVEL_1 (1) // our first API approach

/*
    commands:
    
    SM_CMD_NONE     for internal use only
    SM_CMD_URL      single URL
*/
#define SM_SRV_CMD_NONE		(0)
#define SM_SRV_CMD_URL		(1)

/*
    replies
    
    SM_SRV_RPLY_OK          everything's fine
    SM_SRV_RPLY_FAIL        something goes wrong
    SM_SRV_RPLY_NOREPLY     special value, do not reply
*/
#define SM_SRV_RPLY_OK		(0)
#define SM_SRV_RPLY_FAIL	(-1)
#define SM_SRV_RPLY_NOREPLY	(-2)

/*
    SM_SRV_STRUCT - base struct
*/
typedef struct _SM_SRV_STRUCT
{
    int cmd;        // command
    int structSize; // whole structure size
} SM_SRV_STRUCT;

/*
    SM_SRV_URL_STRUCT

    Struct for SM_SRV_CMD_URL command.
*/
typedef struct _SM_SRV_URL_STRUCT
{
    SM_SRV_STRUCT base; // base.cmd == SM_SRV_CMD_URL

    int urlOffset;  // offset of null-terminated UTF-8 encoded string
    int urlSize;    // number of characters of string pointed by `urlOffset` excluding terminating zero
} SM_SRV_URL_STRUCT;

/*
    SM_SRV_HANDLER - user-specified handler function.
    
    `smStruct` - meaning of this pointer depends on `smStruct->cmd` value

        - For `smStruct->cmd==SM_SRV_CMD_URL` this is pointer to SM_SRV_URL_STRUCT.
    
    return value:
    
    Return value is send back to the caller.
    Use one of `SM_SRV_RPLY_...` defined values.

    - Zero and all positive values indicates success.
    - Negative values indicates failure.
    - Return `SM_SRV_RPLY_NOREPLY` if you don't want send reply.
    
    Notes:
    
    - Hadler will be called from worker thread created by `SMSrvRegister` function.
    - `smStruct` should be considered as read-only pointer.
    - `smStruct` should be considered as valid pointer during handler's call only.
    - You should avoid lengthy operations in handler function.
*/
typedef int (__stdcall *SM_SRV_HANDLER)(SM_SRV_STRUCT* smStruct);

/*
	SMSrvGetWideString - conversion from UTF-8 string to UTF-16 one

	Gets UTF-8 string embedded in SM_SRV_STRUCT-derived structure.

	`smStruct`      - pointer to SM_SRV_STRUCT-derived structure.
	`nOffset`       - offset of the UTF-8 string.
	`nSize`         - length of the UTF-8 string in bytes (characters).
	`pStr           - pointer to user allocated buffer.
	`nStrLen        - number of wide characters in user allocated buffer.

	Return value - number of bytes copied to `pStr`.
*/
int __stdcall SMSrvGetString(SM_SRV_STRUCT* smStruct, int nOffset, int nSize, char* pStr, int nStrLen);

/*
    SMSrvGetWideString - conversion from UTF-8 string to UTF-16 one

    Convert UTF-8 string embedded in SM_SRV_STRUCT-derived structure to UTF-16 string.

    `smStruct`      - pointer to SM_SRV_STRUCT-derived structure.
    `nOffset`       - offset of the UTF-8 string.
    `nSize`         - length of the UTF-8 string in bytes (characters).
    `wStr`          - pointer to user allocated buffer (or NULL see below).
    `nWStrLen       - number of wide characters in user allocated buffer (or zero, see below).

    Return value - number of wide characters copied to `wStr`.

    In order to query required buffer length specify `wStr=NULL` and `nWStrLen=0`.
*/
int __stdcall SMSrvGetWideString(SM_SRV_STRUCT* smStruct, int nOffset, int nSize, wchar_t* wStr, int nWStrLen);

/*
    SMSrvApiLevel - returns highest supported API level.
    
    return value:

    Highest supported API level.
	One of `SM_SRV_API_LEVEL_...` value.
*/
int __stdcall SMSrvApiLevel(void);

/*
    SMSrvRegister - initializes and runs server, registers handler function.
    
    `handler` - address of user-specified handler function
    
    return value:
    
    NULL (zero)         registration failed
    any other value     successfull handler registration, handle required to unregister
*/
void* __stdcall SMSrvRegister(SM_SRV_HANDLER handler);

/*
    SMSrvUnRegister - unregisters handler, stops server.
    
    `handle` - value returned previously by `SMSrvRegister` function
    
    return value:
    
    0                   successfull operation
    any other value     failure

    Notes:

    - `SMSrvRegister` and `SMSrvUnRegister` should be called from the same thread.
    - `handle` parameter could be NULL (zero).
*/
int __stdcall SMSrvUnRegister(void* handle);

#ifdef __cplusplus
}
#endif

#endif // _SM_SRV_H_
