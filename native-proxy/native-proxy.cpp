//
// native-proxy.cpp
//

#include <targetver.h>
#include <SMPipesConfig.h>

#include <Windows.h>
#include <fcntl.h>  
#include <io.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <atlcoll.h>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/reader.h>
#include <rapidjson/error/en.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>

namespace
{
	bool set_binary_mode()
	{
		bool bRes = true;
		int result = _setmode(_fileno(stdin), _O_BINARY);
		if (result == -1)
		{
			bRes = false;
			_tperror(_T("Could not set binary mode for stdin"));
		}

		result = _setmode(_fileno(stdout), _O_BINARY);
		if (result == -1)
		{
			bRes = false;
			_tperror(_T("Could not set binary mode for stdout"));
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

using namespace rapidjson;

int _tmain(int argc, TCHAR* argv[])
{
	if (!set_binary_mode())
	{
		return 1;
	}

	if (!::SetConsoleCtrlHandler(ctrl_handler, true))
	{
		_ftprintf(stderr, _T("Could not set control handler\n"));
		return 2;
	}

	UINT32 nSize, nCurrentSize = 0;
	CAtlArray<char> fs_buffer;
	CAtlArray<char> buffer;

	fs_buffer.SetCount(32 * 1024);

	while (!fBreak)
	{
		if (!fread(&nSize, sizeof(nSize), 1, stdin))
		{
			_ftprintf(stderr, _T("native-proxy: could not read message size\n"));
			fBreak = true;
			continue;
		}

		buffer.SetCount(nSize);
		if (!fread(buffer.GetData(), nSize, 1, stdin))
		{
			_ftprintf(stderr, _T("native-proxy: could not read entire message\n"));
			fBreak = true;
			continue;
		}

		Document msg;
		ParseResult res = msg.Parse(buffer.GetData(), nSize);

		Document reply;
		reply.SetObject();
		Document::AllocatorType& alloc = msg.GetAllocator();
		reply.AddMember("echo", msg, alloc);

		StringBuffer strm;
		Writer<StringBuffer> writer(strm);
		reply.Accept(writer);

		nSize = static_cast<UINT32>(strm.GetSize());
		fwrite(&nSize, sizeof(nSize), 1, stdout);
		fwrite(strm.GetString(), strm.GetSize(), 1, stdout);
		fflush(stdout);
	}

	_ftprintf(stderr, _T("native-proxy: version=%s\n"), _T(SMPIPES_VERSION_STR));
	_ftprintf(stderr, _T("native-proxy: bye\n"));
    return 0;
}

