/*
	critical-section.hpp
*/

#ifndef _CRITICAL_SECTION_HPP_
#define _CRITICAL_SECTION_HPP_

class CCriticalSection
{
public:

	CCriticalSection()
	{
		ZeroMemory(&m_critical_section, sizeof(CRITICAL_SECTION));
		::InitializeCriticalSection(&m_critical_section);
	}

	CCriticalSection(DWORD dwSpinCount)
	{
		ZeroMemory(&m_critical_section, sizeof(CRITICAL_SECTION));
		BOOL fRes = ::InitializeCriticalSectionAndSpinCount(&m_critical_section, dwSpinCount);
		if (!fRes)
		{
			AtlThrowLastWin32();
		}
	}

	~CCriticalSection()
	{
		::DeleteCriticalSection(&m_critical_section);
	}

	void Enter()
	{
		::EnterCriticalSection(&m_critical_section);
	}

	bool TryEnter()
	{
		return ::TryEnterCriticalSection(&m_critical_section) != 0;
	}

	void Leave()
	{
		::LeaveCriticalSection(&m_critical_section);
	}

private:

	CRITICAL_SECTION m_critical_section;
};

class CCriticalSectionLocker
{
public:

	CCriticalSectionLocker(CCriticalSection& critical_section)
		:m_critical_section(critical_section), m_fLock(true)
	{
		critical_section.Enter();
	}

	void Unlock()
	{
		if (m_fLock)
		{
			m_critical_section.Leave();
			m_fLock = false;
		}
	}

	~CCriticalSectionLocker()
	{
		Unlock();
	}

private:

	CCriticalSection& m_critical_section;
	bool m_fLock;
};

class CCriticalSectionSoftLocker
{
public:

	CCriticalSectionSoftLocker(CCriticalSection& critical_section)
		:m_critical_section(critical_section), m_fLock(false)
	{
		m_fLock = critical_section.TryEnter();
	}

	bool IsLocked() const
	{
		return m_fLock;
	}

	void Unlock()
	{
		if (m_fLock)
		{
			m_critical_section.Leave();
			m_fLock = false;
		}
	}

	~CCriticalSectionSoftLocker()
	{
		Unlock();
	}

private:

	CCriticalSection& m_critical_section;
	bool m_fLock;
};


#endif // _CRITICAL_SECTION_HPP_