/*
	chunked-buffer.hpp
*/

#ifndef _CHUNKED_BUFFER_HPP_
#define _CHUNKED_BUFFER_HPP_

typedef CAtlArray<BYTE> Buffer;

class CMemoryBlock
{
public:

	CMemoryBlock(size_t nSize = 0)
		:m_pBlock(nullptr), m_nSize(nSize)
	{
		if (nSize > 0)
		{
			m_pBlock = reinterpret_cast<BYTE*>(malloc(nSize));
			ZeroMemory(m_pBlock, nSize);
		}
	}

	CMemoryBlock(const Buffer& buffer)
		:m_pBlock(nullptr),m_nSize(buffer.GetCount())
	{
		if (m_nSize > 0)
		{
			m_pBlock = reinterpret_cast<BYTE*>(malloc(m_nSize));
			Checked::memcpy_s(m_pBlock, m_nSize, buffer.GetData(), buffer.GetCount());
		}
	}

	CMemoryBlock(const CMemoryBlock& memory_block)
		:m_pBlock(nullptr), m_nSize(memory_block.GetSize())
	{
		if (m_nSize > 0)
		{
			m_pBlock = reinterpret_cast<BYTE*>(malloc(m_nSize));
			Checked::memcpy_s(m_pBlock, m_nSize, memory_block.GetData(), m_nSize);
		}
	}

	~CMemoryBlock()
	{
		free(m_pBlock);
	}

	CMemoryBlock& operator=(const CMemoryBlock& memory_block)
	{
		m_nSize = memory_block.GetSize();
		if (m_pBlock == nullptr)
		{
			m_pBlock = reinterpret_cast<BYTE*>(malloc(m_nSize));
			Checked::memcpy_s(m_pBlock, m_nSize, memory_block.GetData(), m_nSize);
		}
		else
		{
			BYTE* pNewBlock = reinterpret_cast<BYTE*>(realloc(m_pBlock, m_nSize));
			Checked::memcpy_s(pNewBlock, m_nSize, memory_block.GetData(), m_nSize);
			m_pBlock = pNewBlock;
		}

		return *this;
	}

	bool IsEmpty() const
	{
		return m_nSize == 0;
	}

	size_t GetSize() const
	{
		return m_nSize;
	}

	const BYTE* GetData() const
	{
		return m_pBlock;
	}

	void GetBuffer(Buffer& buffer)
	{
		if (m_nSize > 0)
		{
			buffer.SetCount(m_nSize);
			Checked::memcpy_s(buffer.GetData(), m_nSize, m_pBlock, m_nSize);
		}
		else
		{
			buffer.RemoveAll();
		}
	}

	private:

		BYTE* m_pBlock;
		size_t m_nSize;
};

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
		for (size_t i = 0, nCount = m_buffers.GetCount(); i < nCount; ++i)
			res += m_buffers[i].GetCount();
		return res;
	}

	void GetData(Buffer& buffer) const
	{
		size_t nTotalSize = GetCount();
		size_t nBufferSize = nTotalSize;
		buffer.SetCount(nTotalSize);
		BYTE* pBuffer = buffer.GetData();
		for (size_t i = 0, nCount = m_buffers.GetCount(); i < nCount; ++i)
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

	Buffer& GetLastChunk(size_t nBufferSize)
	{
		size_t nCount = m_buffers.GetCount();
		if (nCount == 0)
		{
			m_buffers.SetCount(1);
			m_buffers[0].SetCount(nBufferSize);
			nCount += 1;
		}

		return m_buffers[nCount - 1];
	}

	void TrimLastChunk(size_t nNewSize)
	{
		size_t nCount = m_buffers.GetCount();
		if (nCount == 0) return;

		if (nNewSize > 0)
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
		for (size_t i = 0; i < nSize; ++i)
		{
			m_buffers.GetAt(i).Copy(chunked_buffer.GetChunk(i));
		}
	}
};

#endif // _CHUNKED_BUFFER_HPP_