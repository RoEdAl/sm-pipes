/*
    named-pipe.hpp
*/

#ifndef _NAMED_PIPE_HPP_
#define _NAMED_PIPE_HPP_

class CNamedPipe :public CAtlFile
{
public:

    CNamedPipe() {}
    CNamedPipe(CNamedPipe& namedPipe) :CAtlFile(namedPipe) {}
    explicit CNamedPipe(HANDLE hPipe) :CAtlFile(hPipe) {}

public:

    HRESULT CreateNamedPipe(
        LPCTSTR               lpName,
        DWORD                 dwOpenMode,
        DWORD                 dwPipeMode,
        DWORD                 nMaxInstances = 1,
        DWORD                 nOutBufferSize = 0,
        DWORD                 nInBufferSize = 0,
        DWORD                 nDefaultTimeOut = 500,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes = nullptr
    )
    {
        ATLASSUME(m_h == NULL);

        HANDLE hPipe = ::CreateNamedPipe(
            lpName,
            dwOpenMode,
            dwPipeMode,
            nMaxInstances,
            nOutBufferSize,
            nInBufferSize,
            nDefaultTimeOut,
            lpSecurityAttributes);
        if(hPipe == INVALID_HANDLE_VALUE)
        {
            return AtlHresultFromLastError();
        }

        Attach(hPipe);
        return S_OK;
    }

    HRESULT PeekNamedPipe(
        LPVOID  lpBuffer,
        DWORD   nBufferSize,
        LPDWORD lpBytesRead,
        LPDWORD lpTotalBytesAvail,
        LPDWORD lpBytesLeftThisMessage
    )
    {
        ATLASSUME(m_h != NULL);

        BOOL fResult = ::PeekNamedPipe(
            m_h,
            lpBuffer,
            nBufferSize,
            lpBytesRead,
            lpTotalBytesAvail,
            lpBytesLeftThisMessage
        );

        if(fResult)
        {
            return S_OK;
        }
        else
        {
            return AtlHresultFromLastError();
        }
    }

    HRESULT PeekNamedPipe(DWORD& cbBytesLeftThisMessage)
    {
        ATLASSUME(m_h != NULL);

        BOOL fResult = ::PeekNamedPipe(
            m_h,
            nullptr,
            0,
            nullptr,
            nullptr,
            &cbBytesLeftThisMessage
        );

        if(fResult)
        {
            return S_OK;
        }
        else
        {
            return AtlHresultFromLastError();
        }
    }

    HRESULT ConnectNamedPipe(
        LPOVERLAPPED lpOverlapped = nullptr
    )
    {
        ATLASSUME(m_h != NULL);

        BOOL fResult = ::ConnectNamedPipe(m_h, lpOverlapped);
        if(fResult)
        {
            return S_OK;
        }
        else
        {
            return AtlHresultFromLastError();
        }
    }

    HRESULT DisconnectNamedPipe()
    {
        ATLASSUME(m_h != NULL);
        BOOL fResult = ::DisconnectNamedPipe(m_h);
        if(fResult)
        {
            return S_OK;
        }
        else
        {
            return AtlHresultFromLastError();
        }
    }

    HRESULT SetNamedPipeHandleState(
        LPDWORD lpMode = nullptr,
        LPDWORD lpMaxCollectionCount = nullptr,
        LPDWORD lpCollectDataTimeout = nullptr
    )
    {
        ATLASSUME(m_h != NULL);
        BOOL fResult = ::SetNamedPipeHandleState(
            m_h,
            lpMode,
            lpMaxCollectionCount,
            lpCollectDataTimeout);
        if(fResult)
        {
            return S_OK;
        }
        else
        {
            return AtlHresultFromLastError();
        }
    }

    HRESULT SetNamedPipeHandleState(DWORD dwMode = PIPE_READMODE_MESSAGE)
    {
        ATLASSUME(m_h != NULL);
        BOOL fResult = ::SetNamedPipeHandleState(
            m_h,
            &dwMode,
            nullptr,
            nullptr);
        if(fResult)
        {
            return S_OK;
        }
        else
        {
            return AtlHresultFromLastError();
        }
    }

    HRESULT CancelIo()
    {
        ATLASSUME(m_h != NULL);

        BOOL fResult = ::CancelIo(m_h);
        if(fResult)
        {
            return S_OK;
        }
        else
        {
            return AtlHresultFromLastError();
        }
    }

    HRESULT CancelIoEx(LPOVERLAPPED lpOverlapped = nullptr)
    {
        ATLASSUME(m_h != NULL);

        BOOL fResult = ::CancelIoEx(m_h, lpOverlapped);
        if(fResult)
        {
            return S_OK;
        }
        else
        {
            return AtlHresultFromLastError();
        }
    }
};

#endif // _NAMED_PIPE_HPP_