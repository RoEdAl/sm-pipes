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
#include <named-pipe-defaults.hpp>

#ifdef USE_LOGON_SESSION
    typedef logon_sesssion_security_policy default_security_policy;
#else
    typedef no_security_policy default_security_policy;
#endif

    typedef named_pipe_client<named_pipes_defaults::BUFFER_SIZE> np_client;

    template<size_t S>
    void send_url(np_client& client, const char(&url)[S])
    {
        CStringA json("{\"cmd\":\"url\",\"val\":\"");
        json.Append(url);
        json.Append("\"}");

        size_t nLen = json.GetLength();

        CAtlArray<BYTE> buffer;
        buffer.SetCount(nLen);
        Checked::memcpy_s(buffer.GetData(), nLen, (LPCSTR)json, nLen);

        _tprintf(_T("client: sending URL=%S\n"), url);
        client.SendMessage(buffer);
    }

	class ClientMessages :public pipe_client_basics::INotify
	{
	protected:
		virtual void OnConnect()
		{
			_tprintf(_T("msg: connect\n"));
		}

		virtual void OnMessage(const pipe_client_basics::Buffer& buffer)
		{
            CStringA reply(reinterpret_cast<const char*>(buffer.GetData()), static_cast<int>(buffer.GetCount()));
            _tprintf(_T("msg: %S\n"), (LPCSTR)reply);

			if (m_pClient != nullptr)
			{}
		}

		virtual void OnDisconnect()
		{
			_tprintf(_T("msg: disconnect\n"));
		}

	protected:

		np_client* m_pClient;

	public:

		ClientMessages()
			:m_pClient(nullptr)
		{}

		void SetClient(np_client& client)
		{
			m_pClient = &client;
		}
	};
}

int main()
{
	_tprintf(_T("client: version=%s\n"), _T(SMPIPES_VERSION_STR));
#ifdef USE_LOGON_SESSION
    default_security_policy security_policy;
    if(!security_policy.Init()) return 1;
    CString sPipeName(sm_pipe_name_routines::get_full_pipe_name(security_policy.GetPipeName()));
#else
    CString sPipeName(sm_pipe_name_routines::get_full_pipe_name(default_security_policy::GetPipeName()));
#endif

	ClientMessages messages;
	np_client client(sPipeName, messages);
	messages.SetClient(client);
	_tprintf(_T("client: pipe=%s\n"), (LPCTSTR)client.GetPipeName());

	HRESULT hRes = client.Run();
    _tprintf(_T("client: run %08x\n"), hRes);

	// 1
    send_url(client, "http://poznan.infometeo.pl");
	_gettch();

	// 2
    send_url(client, "http://www.super-memory.com");
	_gettch();

	_tprintf(_T("client: stop\n"));
	client.Stop();

	_gettch();
	_tprintf(_T("client: bye\n"));
    return 0;
}
