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
	typedef json::GenericStringBuffer<json::UTF16<>> WStringBuffer;

	template<size_t S, typename... T>
	void Printf(const TCHAR(&fmt)[S], T... args)
	{
		_fputts(_T("client "), stdout);
		_ftprintf(stdout, fmt, args...);
	}

    void send_msg(np_client& client, const json::Value& msg)
    {
        json::StringBuffer strm;
        json::Writer<json::StringBuffer> writer(strm);
        msg.Accept(writer);

        pipe_client_basics::Buffer buf;
        size_t nBufferSize = strm.GetSize();
        buf.SetCount(nBufferSize);
        Checked::memcpy_s(buf.GetData(), nBufferSize, strm.GetString(), nBufferSize);

		if (nBufferSize <= 512)
		{
			WStringBuffer wbuf;
			json::Writer<WStringBuffer> wwriter(wbuf);
			msg.Accept(wwriter);

			CStringW sMsg(wbuf.GetString(), static_cast<int>(wbuf.GetLength()));
			Printf(_T("> %s\n"), (LPCWSTR)sMsg);
		}
		else
		{
			Printf(_T("> size=%zi\n"), buf.GetCount());
		}

        client.SendMessage(strm.GetString(), nBufferSize);
    }

	class ClientMessages :public pipe_client_basics::INotify
	{
	protected:
		virtual void OnConnect()
		{
			Printf(_T("< CONNECT\n"));
		}

		virtual void OnMessage(const pipe_client_basics::Buffer& buffer)
		{
            CStringA reply(reinterpret_cast<const char*>(buffer.GetData()), static_cast<int>(buffer.GetCount()));
			Printf(_T("< %S\n"), (LPCSTR)reply);

			if (m_pClient != nullptr)
			{}
		}

		virtual void OnDisconnect(bool fOrigin)
		{
			Printf(_T("< DISCONNECT(%s)\n"), fOrigin? _T("origin") : _T("answer"));
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
	Printf(_T(": version=%s\n"), _T(SMPIPES_VERSION_STR));

    json::Document doc;

    if(argc < 2)
    {
        json::ParseResult res = parse_json(doc, IDR_JSON_COMMANDS);

        if(!res)
        {
			Printf(_T("! unable to parse JSON document - offset %u: %S\n"),
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
			Printf(_T("! cannot open file \"%s\" - error code: %d\n"), argv[1], res);
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
				Printf(_T("! unable to parse JSON document - offset %u: %S\n"),
                    (unsigned)res.Offset(),
                    GetParseError_En(res.Code()));
                return 3;
            }
        }
        fclose(f);
    }

    if(!doc.IsArray())
    {
		Printf(_T("! specified JSON document is not an array.\n"));
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
	Printf(_T(": pipe=%s\n"), (LPCTSTR)client.GetPipeName());

	HRESULT hRes = client.Run();
	Printf(_T(": run hr= %08x\n"), hRes);

    if(FAILED(hRes))
    {
        return hRes;
    }

    for(auto& v : doc.GetArray())
    {
        if(!v.IsObject())
        {
			Printf(_T("! skipping non-object array element\n"));
            continue;
        }

        if(!(v.HasMember("cmd") && v.HasMember("val")))
        {
			Printf(_T("! skipping invalid object array element\n"));
            continue;
        }

        send_msg(client, v);

        TCHAR c = _gettch();
        if(c == _T('q'))
        {
			Printf(_T(": exit from loop\n"));
            break;
        }
    }

	Printf(_T(": stop\n"));
	hRes = client.Stop();

	_gettch();
	Printf(_T(": bye, hr= %08x\n"), hRes);
    return hRes;
}
