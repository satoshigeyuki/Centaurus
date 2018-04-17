#pragma once

#include "IPCCommon.hpp"
#include "CodeGenEM64T.hpp"

namespace Centaurus
{
template<class T>
class Stage3Parser : public IPCBase
{
	static constexpr size_t m_stack_size = 64 * 1024 * 1024;
    size_t m_bank_size;
    T& m_chaser;
	const void *m_input;
#if defined(CENTAURUS_BUILD_WINDOWS)
	HANDLE m_thread;
#elif defined(CENTAURUS_BUILD_LINUX)
	pthread_t m_thread;
#endif
private:
#if defined(CENTAURUS_BUILD_WINDOWS)
	static DWORD WINAPI thread_runner(LPVOID param)
#elif defined(CENTAURUS_BUILD_LINUX)
	static void *thread_runner(void *param)
#endif
	{
		Stage3Parser<T> *instance = reinterpret_cast<Stage3Parser<T> *>(param);

		while (true)
		{
			acquire_bank();

		}

#if defined(CENTAURUS_BUILD_WINDOWS)
		ExitThread(0);
#elif defined(CENTAURUS_BUILD_LINUX)
		return NULL;
#endif
	}
public:
	Stage3Parser(T& chaser, size_t bank_size)
		: m_chaser(chaser), m_bank_size(bank_size)
	{
	}
	virtual ~Stage3Parser()
	{
	}
	void run()
	{
		start();
		wait();
	}
	void start()
	{
		clock_t start_time = clock();

#if defined(CENTAURUS_BUILD_WINDOWS)
		m_thread = CreateThread(NULL, m_stack_size, Stage3Parser<T>::thread_runner, (LPVOID)this, 0, NULL);
#elif defined(CENTAURUS_BUILD_LINUX)
		pthread_attr_t attr;

		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, m_stack_size);
		pthread_create(&m_thread, &attr, Stage3Parser<T>::thread_runner, static_cast<void *>(this));
#endif

		clock_t end_time = clock();
	}
	void wait()
	{
#if defined(CENTAURUS_BUILD_WINDOWS)
		WaitForSingleObject(m_thread, INFINITE);
#elif defined(CENTAURUS_BUILD_LINUX)
		pthread_join(m_thread, NULL);
#endif
	}
};
}
