#pragma once

#include "BaseRunner.hpp"
#include "CodeGenEM64T.hpp"

namespace Centaurus
{
template<class T>
class Stage3Runner : public BaseRunner
{
	static constexpr size_t m_stack_size = 64 * 1024 * 1024;
    size_t m_bank_size;
    T& m_chaser;
    int m_current_bank;
private:
#if defined(CENTAURUS_BUILD_WINDOWS)
	static DWORD WINAPI thread_runner(LPVOID param)
#elif defined(CENTAURUS_BUILD_LINUX)
	static void *thread_runner(void *param)
#endif
	{
		Stage3Runner<T> *instance = reinterpret_cast<Stage3Runner<T> *>(param);

        while (true)
        {
            instance->acquire_bank();
            instance->release_bank();
        }
#if defined(CENTAURUS_BUILD_WINDOWS)
		ExitThread(0);
#elif defined(CENTAURUS_BUILD_LINUX)
		return NULL;
#endif
	}
    const void *acquire_bank()
    {
        WindowBankEntry *banks = reinterpret_cast<WindowBankEntry *>(m_sub_window);

        while (true)
        {
            for (int i = 0; i < m_bank_num; i++)
            {
                WindowBankState old_state = WindowBankState::Stage2_Unlocked;

                if (banks[i].state.compare_exchange_weak(old_state, WindowBankState::Stage3_Locked))
                {
                    m_current_bank = i;
                    return (const char *)m_main_window + m_bank_size * i;
                }
            }
        }
    }
    void release_bank()
    {
        WindowBankEntry *banks = reinterpret_cast<WindowBankEntry *>(m_sub_window);

        banks[m_current_bank].state.store(WindowBankState::Free);

        m_current_bank = -1;
    }
public:
	Stage3Runner(T& chaser, size_t bank_size, int bank_num)
		: BaseRunner(bank_size, bank_num), m_chaser(chaser), m_bank_size(bank_size)
	{
	}
	virtual ~Stage3Runner()
	{
	}
	void run()
	{
		start();
		wait();
	}
	void start()
	{
        start(Stage3Runner<T>::thread_runner);
	}
};
}
