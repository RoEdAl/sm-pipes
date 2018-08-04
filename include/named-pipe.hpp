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
        if(!fResult)
        {
            return AtlHresultFromLastError();
        }

        return S_OK;
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

        if(!fResult)
        {
            return AtlHresultFromLastError();
        }

        return S_OK;
    }

    HRESULT ConnectNamedPipe(
        LPOVERLAPPED lpOverlapped = nullptr
    )
    {
        ATLASSUME(m_h != NULL);

        BOOL fResult = ::ConnectNamedPipe(m_h, lpOverlapped);
        if(!fResult)
        {
            return AtlHresultFromLastError();
            
        }

        return S_OK;
    }

    HRESULT DisconnectNamedPipe()
    {
        ATLASSUME(m_h != NULL);

        BOOL fResult = ::DisconnectNamedPipe(m_h);
        if(!fResult)
        {
            return AtlHresultFromLastError();
        }

        return S_OK;
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
        if(!fResult)
        {
            return AtlHresultFromLastError();
        }

        return S_OK;
    }

    HRESULT SetNamedPipeHandleState(DWORD dwMode = PIPE_READMODE_MESSAGE)
    {
        ATLASSUME(m_h != NULL);

        BOOL fResult = ::SetNamedPipeHandleState(
            m_h,
            &dwMode,
            nullptr,
            nullptr);
        if(!fResult)
        {
            return AtlHresultFromLastError();
        }

        return S_OK;
    }

    HRESULT CancelIo()
    {
        ATLASSUME(m_h != NULL);

        BOOL fResult = ::CancelIo(m_h);
        if(!fResult)
        {
            return AtlHresultFromLastError();
        }

        return S_OK;
    }

    HRESULT CancelIoEx(LPOVERLAPPED lpOverlapped = nullptr)
    {
        ATLASSUME(m_h != NULL);

        BOOL fResult = ::CancelIoEx(m_h, lpOverlapped);
        if(!fResult)
        {
            return AtlHresultFromLastError();
        }

        return S_OK;
    }

    template<typename T>
    HRESULT ReadToArray(
        CAtlArray<T>& buffer,
        LPOVERLAPPED pOverlapped)
    {
        ATLASSUME(m_h != NULL);

        BOOL fResult = ::ReadFile(m_h, 
            reinterpret_cast<LPVOID>(buffer.GetData()),
            static_cast<DWORD>(buffer.GetCount() * sizeof(T)),
            nullptr,
            pOverlapped);
        if(!fResult)
        {
            return AtlHresultFromLastError();
        }

        return S_OK;
    }
};

#endif // _NAMED_PIPE_HPP_