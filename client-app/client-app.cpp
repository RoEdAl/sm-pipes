//
// client-app.cpp
//

#include <targetver.h>
#include <SMPipesConfig.h>

#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <atl-headers.h>

namespace
{
#include <named-pipe-client.hpp>
#include <security-policy.hpp>

#ifdef USE_LOGON_SESSION
    typedef logon_sesssion_security_policy default_security_policy;
#else
    typedef no_security_policy default_security_policy;
#endif

	constexpr size_t BUFSIZE = 513;

	class ClientMessages :public pipe_client_basics::INotify
	{
	protected:
		virtual void OnConnect()
		{
			_tprintf(_T("msg: connect\n"));
		}

		virtual void OnMessage(const pipe_client_basics::Buffer& buffer)
		{
			_tprintf(_T("msg: message size=%I64i\n"), buffer.GetCount());
			if (m_pClient != nullptr)
			{
				//m_pClient->SendMessage(buffer);
			}
		}

		virtual void OnDisconnect()
		{
			_tprintf(_T("msg: disconnect\n"));
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

int main()
{
	CAtlArray<BYTE> buffer;
	buffer.SetCount(5 * 1024);
	FillMemory(buffer.GetData(), buffer.GetCount(), 0x54);

	_tprintf(_T("client: version=%s\n"), _T(SMPIPES_VERSION_STR));
#ifdef USE_LOGON_SESSION
    default_security_policy security_policy;
    if(!security_policy.Init()) return 1;
    CString sPipeName(sm_pipe_name_routines::get_full_pipe_name(security_policy.GetPipeName()));
#else
    CString sPipeName(sm_pipe_name_routines::get_full_pipe_name(default_security_policy::GetPipeName()));
#endif

	ClientMessages messages;
	named_pipe_client<BUFSIZE> client(sPipeName, messages);
	messages.SetClient(client);
	_tprintf(_T("client: pipe=%s\n"), (LPCTSTR)client.GetPipeName());

	client.Run();

	// 1
	_tprintf(_T("client: sending first buffer\n"));
	client.SendMessage(buffer);
	_gettch();

	// 2
	_tprintf(_T("client: sending second buffer\n"));
	buffer.SetCount(buffer.GetCount() / 2);
	client.SendMessage(buffer);

	_gettch();
	_tprintf(_T("client: stop\n"));
	client.Stop();

	_gettch();
	_tprintf(_T("client: bye\n"));
    return 0;
}

