/*
    pipe-server-basics.hpp
*/

#ifndef _PIPE_SERVER_BASICS_HPP_
#define _PIPE_SERVER_BASICS_HPP_

#include "named-pipe.hpp"

class pipe_server_basics
{
public:

    static const DWORD PIPE_CONNECT_TIMEOUT = 5000;
#ifdef _DEBUG
    static const DWORD WORKER_THREAD_FINISH_TIMEOUT = 60 * 60 * 1000;
#else
    static const DWORD WORKER_THREAD_FINISH_TIMEOUT = 2000;
#endif
    static const DWORD TERMINATED_THREAD_EXIT_CODE = 0xfffa;

    enum INSTANCE_STATE
    {
        INSTANCE_STATE_CONNECTING,
        INSTANCE_STATE_READING,
        INSTANCE_STATE_WRITING
    };

    typedef ULONG64 INSTANCENO;

protected:

    template<DWORD error_code>
    static bool win32_error(HRESULT hRes)
    {
        return hRes == HRESULT_FROM_WIN32(error_code);
    }

    static bool more_data(HRESULT hRes)
    {
        return win32_error<ERROR_MORE_DATA>(hRes);
    }

    static bool read_succeeded(HRESULT hRes)
    {
        return (SUCCEEDED(hRes) || more_data(hRes));
    }

    static bool abort_succeeded(HRESULT hRes)
    {
        return (SUCCEEDED(hRes) || win32_error<ERROR_OPERATION_ABORTED>(hRes));
    }

    typedef CAtlArray<BYTE> Buffer;

    class CChunkedBuffer
    {
    public:

        CChunkedBuffer()
        {}

        CChunkedBuffer(const Buffer& buffer)
        {
            add_buffer(buffer);
        }

        CChunkedBuffer(const CChunkedBuffer& chunked_buffer)
        {
            copy_chunks(chunked_buffer);
        }

    public:

        size_t GetNumberOfChunks() const
        {
            return m_buffers.GetCount();
        }

        const Buffer& GetChunk(size_t i) const
        {
            return m_buffers.GetAt(i);
        }

        bool IsEmpty() const
        {
            return m_buffers.IsEmpty() || GetCount() == 0;
        }

        size_t GetCount() const
        {
            size_t res = 0;
            for(size_t i = 0, nCount = m_buffers.GetCount(); i < nCount; ++i)
                res += m_buffers[i].GetCount();
            return res;
        }

        void GetData(Buffer& buffer) const
        {
            size_t nTotalSize = GetCount();
            size_t nBufferSize = nTotalSize;
            buffer.SetCount(nTotalSize);
            BYTE* pBuffer = buffer.GetData();
            for(size_t i = 0, nCount = m_buffers.GetCount(); i < nCount; ++i)
            {
                size_t nChunkSize = m_buffers[i].GetCount();
                Checked::memcpy_s(pBuffer, nBufferSize, m_buffers[i].GetData(), nChunkSize);
                pBuffer += m_buffers[i].GetCount();
                nBufferSize -= nChunkSize;
            }
        }

        CChunkedBuffer& operator=(const Buffer& buffer)
        {
            m_buffers.RemoveAll();
            add_buffer(buffer);
            return *this;
        }

        CChunkedBuffer& operator=(const CChunkedBuffer& chunked_buffer)
        {
            m_buffers.RemoveAll();
            copy_chunks(chunked_buffer);
            return *this;
        }

        CChunkedBuffer& operator+=(const Buffer& buffer)
        {
            add_buffer(buffer);
            return *this;
        }

        void Clear()
        {
            m_buffers.RemoveAll();
        }

        void AddBuffer(const Buffer& buffer)
        {
            add_buffer(buffer);
        }

        Buffer& AddBuffer(size_t nBufferSize)
        {
            size_t nCount = m_buffers.GetCount();
            m_buffers.SetCount(nCount + 1);
            m_buffers[nCount].SetCount(nBufferSize);

            return m_buffers.GetAt(nCount);
        }

        void TrimLastChunk(size_t nNewSize)
        {
            size_t nCount = m_buffers.GetCount();
            if(nCount == 0) return;

            if(nNewSize > 0)
            {
                m_buffers.GetAt(nCount - 1).SetCount(nNewSize);
            }
            else
            {
                m_buffers.RemoveAt(nCount - 1);
            }
        }

    protected:

        CAtlArray<Buffer> m_buffers;

    private:

        inline void add_buffer(const Buffer& buffer)
        {
            size_t nCount = m_buffers.GetCount();
            m_buffers.SetCount(nCount + 1);
            m_buffers[nCount].Copy(buffer);
        }

        inline void copy_chunks(const CChunkedBuffer& chunked_buffer)
        {
            size_t nSize = chunked_buffer.GetNumberOfChunks();
            m_buffers.SetCount(nSize);
            for(size_t i = 0; i < nSize; ++i)
            {
                m_buffers.GetAt(i).Copy(chunked_buffer.GetChunk(i));
            }
        }
    };
};

#endif