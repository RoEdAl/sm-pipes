//
// client-app.cpp
//

#include <targetver.h>
#include <SMPipesConfig.h>
#include "resource.h"

#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <atl-headers.h>
#include <rapidjson-headers.hpp>

namespace
{
    namespace json = rapidjson;

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

        if(pszScript != nullptr)
        {
            *pszScript = reinterpret_cast<LPCSTR>(pRes);
        }
        return (dwSize);
    }

    json::ParseResult parse_json(json::Document& doc, UINT nIdrJson)
    {

        LPCSTR psz;
        UINT nLen;

        nLen = get_json_from_resource(nIdrJson, &psz);
        ATLASSERT(nLen > 0);

        return doc.Parse(psz, nLen);
    }
}

namespace
{
    namespace json = rapidjson;

#include <named-pipe-client.hpp>
#include <security-policy.hpp>
#include <named-pipe-defaults.hpp>

#ifdef USE_LOGON_SESSION
    typedef logon_sesssion_security_policy default_security_policy;
#else
    typedef no_security_policy default_security_policy;
#endif

    typedef named_pipe_client<named_pipes_defaults::BUFFER_SIZE> np_client;

    void send_msg(np_client& client, const json::Value& msg)
    {
        json::StringBuffer strm;
        json::Writer<json::StringBuffer> writer(strm);
        msg.Accept(writer);

        pipe_client_basics::Buffer buf;
        size_t nBufferSize = strm.GetSize();
        buf.SetCount(nBufferSize);
        Checked::memcpy_s(buf.GetData(), nBufferSize, strm.GetString(), nBufferSize);

        _tprintf(_T("client: sending next message size=%zi\n"), buf.GetCount());
        client.SendMessage(buf);
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

int _tmain(int argc, TCHAR* argv[])
{
	_tprintf(_T("client: version=%s\n"), _T(SMPIPES_VERSION_STR));

    json::Document doc;

    if(argc < 2)
    {
        json::ParseResult res = parse_json(doc, IDR_JSON_COMMANDS);

        if(!res)
        {
            _tprintf(_T("client: unable to parse JSON document - offset %u: %S\n"),
                (unsigned)res.Offset(),
                GetParseError_En(res.Code()));
            return 3;
        }
    }
    else
    {
        FILE* f;
        errno_t  res = _tfopen_s(&f, argv[1], _T("rb"));

        if(res)
        {
            _tprintf(_T("client: cannot open file \"%s\" - error code: %d\n"), argv[1], res);
            return 2;
        }

        {
            CAtlArray<char> rbuffer;
            rbuffer.SetCount(32 * 1024);

            json::FileReadStream fs(f, rbuffer.GetData(), rbuffer.GetCount());
            json::ParseResult res = doc.ParseStream(fs);

            if(!res)
            {
                fclose(f);
                _tprintf(_T("client: unable to parse JSON document - offset %u: %S\n"),
                    (unsigned)res.Offset(),
                    GetParseError_En(res.Code()));
                return 3;
            }
        }
        fclose(f);
    }

    if(!doc.IsArray())
    {
        _tprintf(_T("client: specified JSON document is not an array.\n"));
        return 4;
    }

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
	_tprintf(_T("client: opening pipe=%s\n"), (LPCTSTR)client.GetPipeName());

	HRESULT hRes = client.Run();
    _tprintf(_T("client: run %08x\n"), hRes);

    if(FAILED(hRes))
    {
        return hRes;
    }

    for(auto& v : doc.GetArray())
    {
        if(!v.IsObject())
        {
            _tprintf(_T("client: skipping non-object array element\n"));
            continue;
        }

        if(!(v.HasMember("cmd") && v.HasMember("val")))
        {
            _tprintf(_T("client: skipping invalid object array element\n"));
            continue;
        }

        send_msg(client, v);

        TCHAR c = _gettch();
        if(c == _T('q'))
        {
            _tprintf(_T("client: exit from loop\n"));
            break;
        }
    }

	_tprintf(_T("client: stop\n"));
	hRes = client.Stop();

	_gettch();
	_tprintf(_T("client: bye - %08x\n"), hRes);
    return hRes;
}
