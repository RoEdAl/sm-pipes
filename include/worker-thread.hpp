/*
	worker-thread.hpp
*/

#ifndef _WORKER_THREAD_HPP_
#define _WORKER_THREAD_HPP_

template<typename T>
class worker_thread
{
private:

#ifdef _DEBUG
	static const DWORD WORKER_THREAD_FINISH_TIMEOUT = 60 * 60 * 1000;
#else
	static const DWORD WORKER_THREAD_FINISH_TIMEOUT = 2000;
#endif

	static const DWORD TERMINATED_THREAD_EXIT_CODE = 0xfffa;

protected:

	CHandle m_evStop;
	CHandle m_thread;

public:

	worker_thread()
	{
		create_event(m_evStop);
	}

	~worker_thread()
	{
		ATLASSERT(m_thread.m_h == nullptr);
	}

	HRESULT Run()
	{
		if (m_thread.m_h != nullptr) return E_FAIL;

		HANDLE hThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, thread_proc, this, CREATE_SUSPENDED, nullptr));
		if (hThread == nullptr)
		{
			return E_FAIL;
		}

		m_thread.Attach(hThread);
		resume_thread(hThread);
		return S_OK;
	}

	HRESULT Stop()
	{
		if (m_thread.m_h == nullptr) return S_FALSE;

		set_event(m_evStop);
		DWORD dwRes = ::WaitForSingleObject(m_thread, WORKER_THREAD_FINISH_TIMEOUT);
		if (dwRes == WAIT_TIMEOUT)
		{
			::TerminateThread(m_thread, TERMINATED_THREAD_EXIT_CODE);
			return S_FALSE;
		}

		m_thread.Close();
		return S_OK;
	}

private:

	static unsigned __stdcall thread_proc(void* _pObj)
	{
		T* pObj = reinterpret_cast<T*>(_pObj);
		_ATLTRY
		{
			pObj->ThreadProc();
			return S_OK;
		}
		_ATLCATCH(e)
		{
			return e.m_hr;
		}
	};

protected:

	static void create_event(CHandle& ev)
	{
		HANDLE hEvent = ::CreateEvent(
			nullptr,    // default security attribute 
			true,    // manual-reset event 
			false,    // initial state = nonsignaled 
			nullptr);   // unnamed event object 

		if (hEvent == nullptr)
		{
			AtlThrowLastWin32();
		}

		ev.Attach(hEvent);
	}

	static void set_event(CHandle& ev)
	{
		BOOL fResult = ::SetEvent(ev);
		if (!fResult)
		{
			AtlThrowLastWin32();
		}
	}

	static void reset_event(CHandle& ev)
	{
		BOOL fResult = ::ResetEvent(ev);
		if (!fResult)
		{
			AtlThrowLastWin32();
		}
	}

	static void resume_thread(HANDLE hThread)
	{
		DWORD dwRes = ::ResumeThread(hThread);
		if (dwRes == ((DWORD)-1))
		{
			AtlThrowLastWin32();
		}
	}
};

#endif // _WORKER_THREAd_HPP_