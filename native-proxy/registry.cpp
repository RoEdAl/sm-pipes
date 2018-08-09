/*
    registry.cpp
*/

#include <targetver.h>
#include <SMPipesConfig.h>
#include "resource.h"
#include <atl-headers.h>

namespace
{
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

	CStringA get_manifest(const CStringA sExeName)
	{
		CStringA sManifest(get_json(IDR_JSON_MANIFEST));

		CStringA s;
		ATLVERIFY(s.LoadString(IDS_APP_HOST_NAME));
		sManifest.Replace("$APPNAME$", s);

		sManifest.Replace("$APPBIN$", sExeName);
		return sManifest;
	}

	CString get_manifest_full_path(LPCTSTR pszAppFullPath)
	{
		CPath path(pszAppFullPath);
		path.RenameExtension(_T(".json"));
		return path;
	}

	CStringA get_exe_bin(LPCTSTR pszAppFullPath)
	{
		CPath path(pszAppFullPath);
		path.StripPath();
		return CStringA(path);
	}

	bool create_manifest_file(const CString& sManifestFullPath, const CStringA& sManifest)
	{
		FILE* f;
		errno_t  res = _tfopen_s(&f, sManifestFullPath, _T("wb"));

		if (res)
		{
			_tprintf(_T("native-proxy: cannot create manifest file \"%s\" - error code: %d\n"), (LPCTSTR)sManifestFullPath, res);
			return false;
		}

		fwrite(sManifest, sManifest.GetLength(), 1, f);
		fclose(f);

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
HRESULT register_chrome_native_msg_host(bool hklm, bool alt, LPCTSTR pszAppFullPath)
{
	CString sManifestPath(get_manifest_full_path(pszAppFullPath));
	CStringA manifest(get_manifest(get_exe_bin(pszAppFullPath)));

	if (!create_manifest_file(sManifestPath, manifest))
	{
		return E_FAIL;
	}

	DWORD dwDisposition;
	CRegKey regThisAppKey;
	CRegKey regNativeMessagingHosts;
	CString sKey;

	ATLVERIFY(sKey.LoadString(IDS_NATIVE_MESSAGING_HOSTS_PATH));
	LONG nRes = regNativeMessagingHosts.Create(hklm ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER,
		sKey,
		REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE | (alt ? ALT_REG_KEY : 0), nullptr,
		&dwDisposition);

	if (nRes != ERROR_SUCCESS)
	{
		_tprintf(_T("native-proxy: cannot open registry key #1 - error code: %d\n"), nRes);
		return AtlHresultFromWin32(nRes);
	}

	ATLVERIFY(sKey.LoadString(IDS_APP_HOST_NAME));
	nRes = regThisAppKey.Create(regNativeMessagingHosts,
		sKey,
		REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE | (alt ? ALT_REG_KEY : 0), nullptr,
		&dwDisposition);

	if (nRes != ERROR_SUCCESS)
	{
		_tprintf(_T("native-proxy: cannot open/create registry key #2 - error code: %d\n"), nRes);
		return AtlHresultFromWin32(nRes);
	}

	nRes = regThisAppKey.SetStringValue(nullptr, sManifestPath);

	if (nRes != ERROR_SUCCESS)
	{
		_tprintf(_T("native-proxy: cannot modify registry key - error code: %d\n"), nRes);
		return AtlHresultFromWin32(nRes);
	}

	return S_OK;
}

HRESULT unregister_chrome_native_msg_host(bool hklm, bool alt)
{
	CRegKey regNativeMessagingHosts;
	CString sKey;
	ATLVERIFY(sKey.LoadString(IDS_NATIVE_MESSAGING_HOSTS_PATH));
	LONG nRes = regNativeMessagingHosts.Open(hklm ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, sKey, KEY_READ | KEY_WRITE | (alt ? ALT_REG_KEY : 0));

	if (nRes != ERROR_SUCCESS)
	{
		if (nRes != ERROR_FILE_NOT_FOUND)
		{
			_tprintf(_T("native-proxy: cannot open registry key #1 - error code: %d\n"), nRes);
			return AtlHresultFromWin32(nRes);
		}
		else
		{
			return S_OK;
		}
	}

	ATLVERIFY(sKey.LoadString(IDS_APP_HOST_NAME));
	nRes = regNativeMessagingHosts.DeleteSubKey(sKey);

	if (nRes != ERROR_SUCCESS && nRes != ERROR_FILE_NOT_FOUND)
	{
		_tprintf(_T("native-proxy: cannot delete subkey - error code: %d\n"), nRes);
		return AtlHresultFromWin32(nRes);
	}

	return S_OK;
}
