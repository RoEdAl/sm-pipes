//
// native-proxy.cpp
//

#include <targetver.h>
#include <SMPipesConfig.h>
#include "resource.h"
#include <atl-headers.h>
#include <fcntl.h>  
#include <io.h>
#include "registry.h"

namespace
{
#include <named-pipe-client.hpp>
#include <security-policy.hpp>
#include <named-pipe-defaults.hpp>

#ifdef USE_LOGON_SESSION
	typedef logon_sesssion_security_policy default_security_policy;
#else
	typedef no_security_policy default_security_policy;
#endif

	const char JSON_CONNECTED[] = "{\"ntrnl\":\"connected\"}";
	const char JSON_DISCONNECTED[] = "{\"ntrnl\":\"disconnected\"}";

	void push_size(size_t size)
	{
		UINT32 nSize = static_cast<UINT32>(size);
		fwrite(&nSize, sizeof(nSize), 1, stdout);
	}

	template<size_t S>
	void push_size_tmpl()
	{
		UINT32 nSize = S;
		fwrite(&nSize, sizeof(nSize), 1, stdout);
	}

	template<size_t S>
	void push_static_msg(const char (&str)[S])
	{
		push_size_tmpl<S>();
		fwrite(str, S, 1, stdout);
		fflush(stdout);
	}

	class ClientMessages :public pipe_client_basics::INotify
	{
	public:
		virtual void OnConnect()
		{
			push_static_msg(JSON_CONNECTED);
			_ftprintf(stderr, _T("native-proxy: connect\n"));
		}

		virtual void OnMessage(const pipe_client_basics::Buffer& buffer)
		{
			if (buffer.GetCount() > 0)
			{
				size_t nSize = buffer.GetCount();
				push_size(nSize);
				fwrite(buffer.GetData(), nSize, 1, stdout);
				fflush(stdout);
			}

			_ftprintf(stderr, _T("native-proxy: msg - size=%I64i\n"), buffer.GetCount());
		}

		virtual void OnDisconnect()
		{
			push_static_msg(JSON_DISCONNECTED);
			_ftprintf(stderr, _T("native-proxy: disconnect\n"));
		}
	};

	bool set_binary_mode()
	{
		bool bRes = true;
		int result = _setmode(_fileno(stdin), _O_BINARY);
		if (result == -1)
		{
			bRes = false;
			_tperror(_T("native-proxy: Could not set binary mode for stdin"));
		}

		result = _setmode(_fileno(stdout), _O_BINARY);
		if (result == -1)
		{
			bRes = false;
			_tperror(_T("native-proxy: Could not set binary mode for stdout"));
		}

		return bRes;
	}
}

int _tmain(int argc, TCHAR* argv[])
{
	if (argc == 2)
	{ // registration
		if (!_tcscmp(argv[1], _T("--register-google-chrome-hklm")))
		{
			return register_chrome_native_msg_host(true, false, argv[0]);
		}
		else if (!_tcscmp(argv[1], _T("--register-google-chrome-hkcu")))
		{
			return register_chrome_native_msg_host(false, false, argv[0]);
		}
		else if (!_tcscmp(argv[1], _T("--unregister-google-chrome-hklm")))
		{
			return unregister_chrome_native_msg_host(true, false);
		}
		else if (!_tcscmp(argv[1], _T("--unregister-google-chrome-hkcu")))
		{
			return unregister_chrome_native_msg_host(false, false);
		}
		if (!_tcscmp(argv[1], _T("--alt-register-google-chrome-hklm")))
		{
			return register_chrome_native_msg_host(true, true, argv[0]);
		}
		else if (!_tcscmp(argv[1], _T("--alt-register-google-chrome-hkcu")))
		{
			return register_chrome_native_msg_host(false, true, argv[0]);
		}
		else if (!_tcscmp(argv[1], _T("--alt-unregister-google-chrome-hklm")))
		{
			return unregister_chrome_native_msg_host(true, true);
		}
		else if (!_tcscmp(argv[1], _T("--alt-unregister-google-chrome-hkcu")))
		{
			return unregister_chrome_native_msg_host(false, true);
		}
		else if (!_tcscmp(argv[1], _T("--register")) || !_tcscmp(argv[1], _T("--register-info")))
		{
			CString sDesc;
			ATLVERIFY(sDesc.LoadString(IDS_REGISTER_GOOGLE_CHROME_INFO));
			_fputts(sDesc, stdout);
			_fputtc(_T('\n'), stdout);
			return 0;
		}
	}

	if (!set_binary_mode())
	{
		return 1;
	}

	_ftprintf(stderr, _T("native-proxy: version=%s\n"), _T(SMPIPES_VERSION_STR));

#ifdef USE_LOGON_SESSION
	default_security_policy security_policy;
	if (!security_policy.Init()) return 1;
	CString sPipeName(sm_pipe_name_routines::get_full_pipe_name(security_policy.GetPipeName()));
#else
	CString sPipeName(sm_pipe_name_routines::get_full_pipe_name(default_security_policy::GetPipeName()));
#endif

	ClientMessages messages;
	named_pipe_client<named_pipes_defaults::BUFFER_SIZE> client(sPipeName, messages);

	_ftprintf(stderr, _T("native-proxy: pipe=%s\n"), (LPCTSTR)client.GetPipeName());

	client.Run();

	bool fBreak = false;
	UINT32 nSize, nCurrentSize = 0;
	CAtlArray<BYTE> buffer;

	_ftprintf(stderr, _T("native-proxy: main loop\n"));
	while (!fBreak)
	{
		// read size
		if (!fread(&nSize, sizeof(nSize), 1, stdin))
		{
			if (!feof(stdin))
			{
				_ftprintf(stderr, _T("native-proxy: could not read message size\n"));
			}
			fBreak = true;
			continue;
		}

		// read message
		buffer.SetCount(nSize);
		if (!fread(reinterpret_cast<char*>(buffer.GetData()), nSize, 1, stdin))
		{
			_ftprintf(stderr, _T("native-proxy: could not read entire message\n"));
			fBreak = true;
			continue;
		}

		// just send received message
		client.SendMessage(buffer);
	}

	_ftprintf(stderr, _T("native-proxy: stop\n"));
	HRESULT hRes = client.Stop();

	_ftprintf(stderr, _T("native-proxy: bye - %08x\n"), hRes);
    return hRes;
}
