//
// native-proxy.cpp
//

#include <targetver.h>
#include <SMPipesConfig.h>
#include <atl-headers.h>

#include <Windows.h>
#include <fcntl.h>  
#include <io.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <atlcoll.h>

namespace
{
#include <named-pipe-client.hpp>
#include <security-policy.hpp>

#ifdef USE_LOGON_SESSION
	typedef logon_sesssion_security_policy default_security_policy;
#else
	typedef no_security_policy default_security_policy;
#endif

	constexpr size_t BUFSIZE = 479;

	class ClientMessages :public pipe_client_basics::INotify
	{
	protected:
		virtual void OnConnect()
		{
			_ftprintf(stderr, _T("native-proxy: connect\n"));
		}

		virtual void OnMessage(const pipe_client_basics::Buffer& buffer)
		{
			_ftprintf(stderr, _T("native-proxy: msg - size=%I64i\n"), buffer.GetCount());

			if (buffer.GetCount() > 0)
			{
				UINT32 nSize = static_cast<UINT32>(buffer.GetCount());
				fwrite(&nSize, sizeof(nSize), 1, stdout);
				fwrite(buffer.GetData(), nSize, 1, stdout);
				fflush(stdout);
			}

			if (m_pClient != nullptr)
			{
				//m_pClient->SendMessage(buffer);
			}
		}

		virtual void OnDisconnect()
		{
			_ftprintf(stderr, _T("native-proxy: disconnect\n"));
		}

	protected:

		named_pipe_client<BUFSIZE>* m_pClient;

	public:

		ClientMessages()
			:m_pClient(nullptr)
		{}

		void SetClient(named_pipe_client<BUFSIZE>& client)
		{
			m_pClient = &client;
		}
	};
}


namespace
{
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

	bool fBreak = false;

	BOOL WINAPI ctrl_handler(DWORD fdwCtrlType)
	{
		switch (fdwCtrlType)
		{
			// Handle the CTRL-C signal. 
		case CTRL_C_EVENT:
			_ftprintf(stderr, _T("Ctrl-C event\n\n"));
			fBreak = true;
			return(true);

			// CTRL-CLOSE: confirm that the user wants to exit. 
		case CTRL_CLOSE_EVENT:
			_ftprintf(stderr, _T("Ctrl-Close event\n\n"));
			return(true);

			// Pass other signals to the next handler. 
		case CTRL_BREAK_EVENT:
			_ftprintf(stderr, _T("Ctrl-Break event\n\n"));
			fBreak = true;
			return (false);

		case CTRL_LOGOFF_EVENT:
			_ftprintf(stderr, _T("Ctrl-Logoff event\n\n"));
			return (false);

		case CTRL_SHUTDOWN_EVENT:
			_ftprintf(stderr, _T("Ctrl-Shutdown event\n\n"));
			return (false);

		default:
			return (false);
		}
	}
}

int _tmain(int argc, TCHAR* argv[])
{
	if (!set_binary_mode())
	{
		return 1;
	}

	if (!::SetConsoleCtrlHandler(ctrl_handler, true))
	{
		_ftprintf(stderr, _T("native-proxy: Could not set control handler\n"));
		return 2;
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
	named_pipe_client<BUFSIZE> client(sPipeName, messages);
	messages.SetClient(client);
	_ftprintf(stderr, _T("native-proxy: pipe=%s\n"), (LPCTSTR)client.GetPipeName());

	client.Run();

	UINT32 nSize, nCurrentSize = 0;
	CAtlArray<char> fs_buffer;
	CAtlArray<BYTE> buffer;

	fs_buffer.SetCount(32 * 1024);

	while (!fBreak)
	{
		if (!fread(&nSize, sizeof(nSize), 1, stdin))
		{
			if (!feof(stdin))
			{
				_ftprintf(stderr, _T("native-proxy: could not read message size\n"));
			}
			fBreak = true;
			continue;
		}

		buffer.SetCount(nSize);
		if (!fread(reinterpret_cast<char*>(buffer.GetData()), nSize, 1, stdin))
		{
			_ftprintf(stderr, _T("native-proxy: could not read entire message\n"));
			fBreak = true;
			continue;
		}

		client.SendMessage(buffer);
	}

	_ftprintf(stderr, _T("native-proxy: stop\n"));
	client.Stop();
	_ftprintf(stderr, _T("native-proxy: bye\n"));
    return 0;
}

