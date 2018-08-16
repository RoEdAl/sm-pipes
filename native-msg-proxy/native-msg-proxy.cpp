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
#include "printf.hpp"

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
	void push_size()
	{
		UINT32 nSize = S;
		fwrite(&nSize, sizeof(nSize), 1, stdout);
	}

	template<size_t S>
	void push_static_msg(const char (&str)[S])
	{ // do not send tailing zero character
		push_size<S-1>();
		fwrite(str, S-1, 1, stdout);
		fflush(stdout);
	}

	class ClientMessages :public pipe_client_basics::INotify
	{
	public:
		virtual void OnConnect()
		{
			push_static_msg(JSON_CONNECTED);
			Printf(_T("< CONNECT\n"));
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

			Printf(_T("< size=%zi\n"), buffer.GetCount());
		}

		virtual void OnDisconnect(bool fOrigin)
		{
            if(!fOrigin)
            {
                push_static_msg(JSON_DISCONNECTED);
            }

			Printf(_T("< DISCONNECT(%s)\n"), fOrigin? _T("origin") : _T("answer"));
		}
	};

	bool set_binary_mode()
	{
		bool bRes = true;
		int result = _setmode(_fileno(stdin), _O_BINARY);
		if (result == -1)
		{
			bRes = false;
			_tperror(_T("! Could not set binary mode for stdin"));
		}

		result = _setmode(_fileno(stdout), _O_BINARY);
		if (result == -1)
		{
			bRes = false;
			_tperror(_T("! Could not set binary mode for stdout"));
		}

		return bRes;
	}
}

int _tmain(int argc, TCHAR* argv[])
{
	if (argc == 2)
	{ // registration
		if (!_tcscmp(argv[1], _T("--register-google-hklm")))
		{
			return register_native_msg_host(true, true, false);
		}
		else if (!_tcscmp(argv[1], _T("--register-google-hkcu")))
		{
			return register_native_msg_host(true, false, false);
		}
		else if (!_tcscmp(argv[1], _T("--unregister-google-hklm")))
		{
			return unregister_native_msg_host(true, true, false);
		}
		else if (!_tcscmp(argv[1], _T("--unregister-google-hkcu")))
		{
			return unregister_native_msg_host(true, false, false);
		}
		if (!_tcscmp(argv[1], _T("--alt-register-google-hklm")))
		{
			return register_native_msg_host(true, true, true);
		}
		else if (!_tcscmp(argv[1], _T("--alt-register-google-hkcu")))
		{
			return register_native_msg_host(true, false, true);
		}
		else if (!_tcscmp(argv[1], _T("--alt-unregister-google-hklm")))
		{
			return unregister_native_msg_host(true, true, true);
		}
		else if (!_tcscmp(argv[1], _T("--alt-unregister-google-hkcu")))
		{
			return unregister_native_msg_host(true, false, true);
		}
        else if(!_tcscmp(argv[1], _T("--register-mozilla-hklm")))
        {
            return register_native_msg_host(false, true, false);
        }
        else if(!_tcscmp(argv[1], _T("--register-mozilla-hkcu")))
        {
            return register_native_msg_host(false, false, false);
        }
        else if(!_tcscmp(argv[1], _T("--unregister-mozilla-hklm")))
        {
            return unregister_native_msg_host(false, true, false);
        }
        else if(!_tcscmp(argv[1], _T("--unregister-mozilla-hkcu")))
        {
            return unregister_native_msg_host(false, false, false);
        }
        if(!_tcscmp(argv[1], _T("--alt-register-mozilla-hklm")))
        {
            return register_native_msg_host(false, true, true);
        }
        else if(!_tcscmp(argv[1], _T("--alt-register-mozilla-hkcu")))
        {
            return register_native_msg_host(false, false, true);
        }
        else if(!_tcscmp(argv[1], _T("--alt-unregister-mozilla-hklm")))
        {
            return unregister_native_msg_host(false, true, true);
        }
        else if(!_tcscmp(argv[1], _T("--alt-unregister-mozilla-hkcu")))
        {
            return unregister_native_msg_host(false, false, true);
        }
		else if (!_tcscmp(argv[1], _T("--register")) || !_tcscmp(argv[1], _T("--register-info")))
		{
			CString sDesc;
			ATLVERIFY(sDesc.LoadString(IDS_REGISTER_GOOGLE_INFO));
			_fputts(sDesc, stdout);
			_fputts(_T("\n\n"), stdout);
            ATLVERIFY(sDesc.LoadString(IDS_REGISTER_MOZILLA_INFO));
            _fputts(sDesc, stdout);
            _fputtc(_T('\n'), stdout);
			return 0;
		}
	}

	if (!set_binary_mode())
	{
		return 1;
	}

	Printf(_T(": version=%s\n"), _T(SMPIPES_VERSION_STR));

#ifdef USE_LOGON_SESSION
	default_security_policy security_policy;
	if (!security_policy.Init()) return 1;
	CString sPipeName(sm_pipe_name_routines::get_full_pipe_name(security_policy.GetPipeName()));
#else
	CString sPipeName(sm_pipe_name_routines::get_full_pipe_name(default_security_policy::GetPipeName()));
#endif

	ClientMessages messages;
	named_pipe_client<named_pipes_defaults::BUFFER_SIZE> client(sPipeName, messages);

	Printf(_T(": pipe=%s\n"), (LPCTSTR)client.GetPipeName());

	HRESULT hRes = client.Run();
	Printf(_T(": run hr=%08x\n"), hRes);

	bool fBreak = false;
	UINT32 nSize, nCurrentSize = 0;
	CAtlArray<BYTE> buffer;

	while (!fBreak)
	{
		// read size
		if (!fread(&nSize, sizeof(nSize), 1, stdin))
		{
			if (!feof(stdin))
			{
				Printf(_T("! could not read message size\n"));
			}
			fBreak = true;
			continue;
		}

		// read message
		buffer.SetCount(nSize);
		if (!fread(reinterpret_cast<char*>(buffer.GetData()), nSize, 1, stdin))
		{
			Printf(_T("! could not read entire message\n"));
			fBreak = true;
			continue;
		}

		Printf(_T("> message received, size=%d\n"), nSize);

		// just send received message
		client.SendMessage(buffer);
	}

	Printf(_T(": stop\n"));
	hRes = client.Stop();

	Printf(_T(": bye hr=%08x\n"), hRes);
    return hRes;
}
