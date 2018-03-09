#pragma once

#if defined(CENTAURUS_BUILD_WINDOWS)
#include <Windows.h>
#elif defined(CENTAURUS_BUILD_LINUX)
#include <pthread.h>
#include <unistd.h>
#endif

namespace Centaurus
{
template<class T>
class MasterParser
{
    T& m_parser;
    const void *m_memory, *m_result;
private:
#if defined(CENTAURUS_BUILD_WINDOWS)
    static DWORD WINAPI thread_runner(LPVOID param)
    {
        MasterParser<T> *instance = reinterpret_cast<MasterParser<T> *>(param);

        instance->m_result = instance->m_parser(instance->m_memory);

        ExitThread(0);
    }
#elif defined(CENTAURUS_BUILD_LINUX)
    static void *thread_runner(void *param)
    {
        MasterParser<T> *instance = reinterpret_cast<MasterParser<T> *>(param);

        instance->m_result = instance->m_parser(instance->m_memory);
    }
#endif
public:
    MasterParser(T& parser, const void *memory, const void *result)
        : m_parser(parser), m_memory(memory), m_result(result)
    {
    }
    void run()
    {
        clock_t start_time = clock();

#if defined(CENTAURUS_BUILD_WINDOWS)
        HANDLE hThread = CreateThread(NULL, 256*1024*1024, MasterParser<T>::thread_runner, (LPVOID)this, 0, NULL);

        WaitForSingleObject(hThread, INFINITE);
#elif defined(CENTAURUS_BUILD_LINUX)
        pthread_t thread;
        pthread_attr_t attr;

        pthread_attr_init(&attr);

        pthread_attr_setstacksize(&attr, 256 * 1024 * 1024);

        pthread_create(&thread, &attr, MasterParser<T>::thread_runner, this);

        pthread_join(thread, NULL);
#endif

        clock_t end_time = clock();

        //char buf[64];
        //snprintf(buf, 64, "Elapsed time = %lf[ms]\r\n", (double)(end_time - start_time) * 1000.0 / CLOCKS_PER_SEC);
        //Logger::WriteMessage(buf);
    }
    const void *get_result()
    {
        return m_result;
    }
};
}
