#pragma once

#include "BaseRunner.hpp"
#include "CodeGenEM64T.hpp"

namespace Centaurus
{
template<class T>
class Stage3Runner : public BaseRunner
{
    size_t m_bank_size;
    T& m_chaser;
    const uint64_t *m_window;
    int m_position;
    int m_current_bank;
    int m_counter;
private:
#if defined(CENTAURUS_BUILD_WINDOWS)
	static DWORD WINAPI thread_runner(LPVOID param)
#elif defined(CENTAURUS_BUILD_LINUX)
	static void *thread_runner(void *param)
#endif
	{
		Stage3Runner<T> *instance = reinterpret_cast<Stage3Runner<T> *>(param);

        instance->m_current_bank = -1;
        instance->m_counter = 0;

        while (true)
        {
            m_window = reinterpret_cast<const uint64_t *>(instance->acquire_bank());

            if (m_window == NULL) break;

            instance->release_bank();
        }
#if defined(CENTAURUS_BUILD_WINDOWS)
		ExitThread(0);
#elif defined(CENTAURUS_BUILD_LINUX)
		return NULL;
#endif
	}
    void reduce()
    {
        std::vector<SVCapsule> values;
        CSTMarker start_marker(m_window[m_position]);

        for (int i = m_position + 1; i < m_bank_size / 8; i++)
        {
            CSTMarker marker(m_window[m_position]);

            if (marker.is_start_marker())
            {
                reduce();
            }
            else if (marker.is_end_marker())
            {

                return;
            }
            else if (marker.is_sv_marker())
            {

            }
            else
            {

            }
        }
    }
    void *acquire_bank()
    {
        WindowBankEntry *banks = reinterpret_cast<WindowBankEntry *>(m_sub_window);

        while (true)
        {
            for (int i = 0; i < m_bank_num; i++)
            {
                if (banks[i].state == WindowBankState::Stage2_Unlocked)
                {
                    if (banks[i].number == m_counter)
                    {
                        banks[i].state = WindowBankState::Stage3_Locked;

                        std::cout << "Bank " << banks[i].number << " reached Stage3" << std::endl;

                        m_current_bank = i;
                        m_counter++;
                        return (char *)m_main_window + m_bank_size * i;
                    }
                }
                else if (banks[i].state == WindowBankState::YouAreDone)
                {
                    return NULL;
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
	Stage3Runner(const char *filename, T& chaser, size_t bank_size, int bank_num, int master_pid)
		: BaseRunner(filename, bank_size, bank_num, master_pid), m_chaser(chaser), m_bank_size(bank_size)
	{
#if defined(CENTAURUS_BUILD_WINDOWS)
		m_mem_handle = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, m_memory_name);

		m_sub_window = MapViewOfFile(m_mem_handle, FILE_MAP_ALL_ACCESS, 0, 0, get_sub_window_size());

		m_main_window = MapViewOfFile(m_mem_handle, FILE_MAP_ALL_ACCESS, 0, get_sub_window_size(), get_main_window_size());
#elif defined(CENTAURUS_BUILD_LINUX)
		int fd = shm_open(m_memory_name, O_RDWR, 0666);

		ftruncate(fd, get_window_size());

		m_sub_window = mmap(NULL, get_sub_window_size(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

		m_main_window = mmap(NULL, get_main_window_size(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, get_sub_window_size());
#endif
	}
	virtual ~Stage3Runner()
	{
#if defined(CENTAURUS_BUILD_WINDOWS)
		if (m_main_window != NULL)
			UnmapViewOfFile(m_main_window);
		if (m_sub_window != NULL)
			UnmapViewOfFile(m_sub_window);
		if (m_mem_handle != NULL)
			CloseHandle(m_mem_handle);
#elif defined(CENTAURUS_BUILD_LINUX)
		if (m_main_window != MAP_FAILED)
			munmap(m_main_window, get_main_window_size());
		if (m_sub_window != MAP_FAILED)
			munmap(m_sub_window, get_sub_window_size());
#endif
	}
	void start()
	{
        _start(Stage3Runner<T>::thread_runner, this);
	}
};
}
