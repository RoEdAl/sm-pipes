/*
    registry.cpp
*/

#include <targetver.h>
#include <SMPipesConfig.h>
#include "resource.h"
#include <atl-headers.h>
#include <errno.h>  

namespace
{
#include "printf.hpp"

	HRESULT get_module_full_path(CString& strAppDir)
	{
		if (0 != ::GetModuleFileName(NULL, CStrBuf(strAppDir, MAX_PATH), MAX_PATH))
		{
			return S_OK;
		}
		else
		{
			return AtlHresultFromLastError();
		}
	}

    HRESULT get_module_full_path(CPath& path)
    {
        return get_module_full_path(path.m_strPath);
    }

	UINT get_json_from_resource(UINT nIdrJson, LPCSTR* pszScript)
	{
		HMODULE hThisModule = ::GetModuleHandle(nullptr);

		HRSRC hSrc = ::FindResource(hThisModule, MAKEINTRESOURCE(nIdrJson), _T("JSON"));
		ATLASSERT(hSrc != nullptr);

		HGLOBAL hRes = ::LoadResource(hThisModule, hSrc);
		ATLASSERT(hRes != nullptr);

		DWORD dwSize = ::SizeofResource(hThisModule, hSrc);
		ATLASSERT(dwSize > 0);

		LPVOID pRes = ::LockResource(hRes);
		ATLASSERT(pRes != nullptr);

		if (pszScript != nullptr)
		{
			*pszScript = reinterpret_cast<LPCSTR>(pRes);
		}
		return (dwSize);
	}

	CStringA get_json(UINT nIdrJson)
	{
		LPCSTR psz;
		UINT nLen;
		CStringA res;

		nLen = get_json_from_resource(nIdrJson, &psz);

		if (nLen > 0)
		{
			res.SetString(psz, nLen);
		}

		return res;
	}

	CStringA get_manifest(bool google, const CStringA& sExeName)
	{
		CStringA sManifest(get_json(google? IDR_JSON_MANIFEST_GOOGLE : IDR_JSON_MANIFEST_MOZILLA));

		CStringA s;
		ATLVERIFY(s.LoadString(google? IDS_APP_HOST_GOOGLE_NAME : IDS_APP_HOST_MOZILLA_NAME));
		sManifest.Replace("$APPNAME$", s);
		sManifest.Replace("$APPBIN$", sExeName);
		return sManifest;
	}

	CString get_manifest_full_path(bool google, const CPath& exeFullPath)
	{
		CPath path(exeFullPath);
        CPath npath(exeFullPath);
        path.RemoveFileSpec();
        npath.StripPath();
        npath.RemoveExtension();
        npath.m_strPath += google ? _T("-GoggleChrome") : _T("-Mozilla");
		npath.RenameExtension(_T(".json"));
        path += npath;
        return path;
	}

	CStringA get_exe_bin(const CPath& exeFullPath)
	{
		CPath path(exeFullPath);
		path.StripPath();
		return CStringA(path);
	}

	bool create_manifest_file(const CPath& manifestFullPath, const CStringA& sManifest)
	{
		FILE* f;
        errno_t  res;

        res = _taccess_s(manifestFullPath, 02);
        if(res == EACCES)
        {
            res = _tchmod(manifestFullPath, _S_IREAD| _S_IWRITE);
            if(res == -1)
            {
                res = errno;
                Printf(_T("! cannot change permissions of file \"%s\" - error code: %d\n"), (LPCTSTR)manifestFullPath, res);
                return false;
            }
        }

		res = _tfopen_s(&f, manifestFullPath, _T("wb"));

		if (res)
		{
			Printf(_T("! cannot create manifest file \"%s\" - error code: %d\n"), (LPCTSTR)manifestFullPath, res);
			return false;
		}

		fwrite(sManifest, sManifest.GetLength(), 1, f);
		res = fclose(f);

        // make manifest read-only
        res = _tchmod(manifestFullPath, _S_IREAD);
        if(res == -1)
        {
            res = errno;
            Printf(_T("! cannot change permissions of file \"%s\" - error code: %d\n"), (LPCTSTR)manifestFullPath, res);
            return false;
        }

		return true;
	}

#if defined(SMPIPES_PLATFORM_32)
	constexpr DWORD ALT_REG_KEY = KEY_WOW64_64KEY;
#elif defined(SMPIPES_PLATFORM_64)
	constexpr DWORD ALT_REG_KEY = KEY_WOW64_32KEY;
#else
	constexpr DWORD ALT_REG_KEY = (0);
#endif
}

// HKLM or HKCU
HRESULT register_native_msg_host(bool google, bool hklm, bool alt)
{
    CPath exeFullPath;
    HRESULT hRes = get_module_full_path(exeFullPath);
    if(FAILED(hRes))
    {
        return hRes;
    }

	CPath manifestPath(get_manifest_full_path(google, exeFullPath));
	CStringA manifest(get_manifest(google, get_exe_bin(exeFullPath)));

	if (!create_manifest_file(manifestPath, manifest))
	{
		return E_FAIL;
	}

	DWORD dwDisposition;
	CRegKey regThisAppKey;
	CRegKey regNativeMessagingHosts;
	CString sKey;

	ATLVERIFY(sKey.LoadString(google? IDS_NATIVE_MESSAGING_GOOGLE_HOSTS_PATH : IDS_NATIVE_MESSAGING_MOZILLA_HOSTS_PATH));
	LONG nRes = regNativeMessagingHosts.Create(hklm ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER,
		sKey,
		REG_NONE, REG_OPTION_NON_VOLATILE,
		KEY_READ | KEY_WRITE | (alt ? ALT_REG_KEY : 0),
		nullptr,
		&dwDisposition);

	if (nRes != ERROR_SUCCESS)
	{
		Printf(_T("! cannot open registry key #1 - error code: %d\n"), nRes);
		return AtlHresultFromWin32(nRes);
	}

	ATLVERIFY(sKey.LoadString(google? IDS_APP_HOST_GOOGLE_NAME : IDS_APP_HOST_MOZILLA_NAME));
    nRes = regNativeMessagingHosts.SetKeyValue(sKey, manifestPath);
	if (nRes != ERROR_SUCCESS)
	{
		Printf(_T("! cannot modify registry key - error code: %d\n"), nRes);
		return AtlHresultFromWin32(nRes);
	}

	return S_OK;
}

HRESULT unregister_native_msg_host(bool google, bool hklm, bool alt)
{
	CRegKey regNativeMessagingHosts;
	CString sKey;
    ATLVERIFY(sKey.LoadString(google ? IDS_NATIVE_MESSAGING_GOOGLE_HOSTS_PATH : IDS_NATIVE_MESSAGING_MOZILLA_HOSTS_PATH));
	LONG nRes = regNativeMessagingHosts.Open(hklm ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, sKey, KEY_READ | KEY_WRITE | (alt ? ALT_REG_KEY : 0));

	if (nRes != ERROR_SUCCESS)
	{
		if (nRes != ERROR_FILE_NOT_FOUND)
		{
			Printf(_T("! cannot open registry key #1 - error code: %d\n"), nRes);
			return AtlHresultFromWin32(nRes);
		}
		else
		{
			return S_OK;
		}
	}

	ATLVERIFY(sKey.LoadString(google? IDS_APP_HOST_GOOGLE_NAME : IDS_APP_HOST_MOZILLA_NAME));
	nRes = regNativeMessagingHosts.DeleteSubKey(sKey);

	if (nRes != ERROR_SUCCESS && nRes != ERROR_FILE_NOT_FOUND)
	{
		Printf(_T("! cannot delete subkey - error code: %d\n"), nRes);
		return AtlHresultFromWin32(nRes);
	}

	return S_OK;
}
