/*
    str-encode.cpp
*/

#include <targetver.h>
#include <SMPipesConfig.h>
#include <windows.h>
#include <sm-srv.h>

namespace
{
    int utf8_to_utf16(const char* pUTf8, int nUTF8Len, wchar_t* pUTF16, int nUTF16Len)
    {
        if(nUTF8Len <= 0) {
            return 0;
        }

        int nLen16;

        if((nLen16 = ::MultiByteToWideChar(CP_UTF8, 0, pUTf8, nUTF8Len, nullptr, 0)) == 0)
        {
            return -1; //conversion error!
        }

        if(pUTF16 == nullptr && nUTF16Len == 0)
        {
            return nLen16;
        }

        if((nLen16+1) > nUTF16Len)
        {
            return -2;
        }

        pUTF16[nLen16] = L'\0';
        nLen16 = ::MultiByteToWideChar(CP_UTF8, 0, pUTf8, nUTF8Len, pUTF16, nLen16);

        return nLen16;
    }
}

extern "C" int __stdcall SMSrvGetWideString(SM_SRV_STRUCT* pStruct, int nOffset, int nSize, wchar_t* pWStr, int nWStrLen)
{
    if(pStruct == nullptr || nOffset < sizeof(SM_SRV_STRUCT) || nSize < 0)
    {
        return -1;
    }

    if(pStruct->structSize < (nOffset + nSize + 1))
    {
        return -1;
    }

    char* pBuf = reinterpret_cast<char*>(pStruct);
    pBuf += nOffset;

    if(pBuf[nSize] != '\0')
    {
        return -2;
    }

    return utf8_to_utf16(pBuf, nSize, pWStr, nWStrLen);
}