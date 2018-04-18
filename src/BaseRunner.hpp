#pragma once

#if defined(CENTAURUS_BUILD_WINDOWS)
#include <Windows.h>
#elif defined(CENTAURUS_BUILD_LINUX)
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#endif

#include <stdio.h>
#include <atomic>

#define ALIGN_NEXT(x, a) (((x) + (a) - 1) / (a) * (a))
#define PROGRAM_UUID "{57DF45C9-6D0C-4DD2-9B41-B71F8CF66B13}"
#define PROGRAM_NAME "Centaurus"
#define IPC_PAGESIZE (8 * 1024 * 1024)

namespace Centaurus
{
class CSTMarker
{
	uint64_t m_value;
public:
	CSTMarker(uint64_t value) : m_value(value) {}
	int get_machine_id() const
	{
		return (m_value >> 48) & 0x7FFF;
	}
	bool is_start_marker() const
	{
		return m_value & ((uint64_t)1 << 63);
	}
	bool is_end_marker() const
	{
		return !(m_value & ((uint64_t)1 << 63));
	}
	uint64_t get_offset() const
	{
		return m_value & (((uint64_t)1 << 48) - 1);
	}
	const void *offset_ptr(const void *p) const
	{
		return static_cast<void *>((char *)p + get_offset());
	}
};
class BaseRunner
{
protected:
	enum class WindowBankState
	{
		Free,
		Stage1_Locked,
		Stage1_Unlocked,
		Stage2_Locked,
		Stage2_Unlocked,
		Stage3_Locked
	};
	struct WindowBankEntry
	{
		int number;
		std::atomic<WindowBankState> state;
	};
	const void *m_input_window;
    size_t m_input_size;
    void *m_main_window, *m_sub_window;
    size_t m_bank_size;
	int m_bank_num;
	size_t m_stack_size;
#if defined(CENTAURUS_BUILD_WINDOWS)
    HANDLE m_slave_lock;
	HANDLE m_thread;
	char m_memory_name[MAX_PATH], m_slave_lock_name[MAX_PATH];
#elif defined(CENTAURUS_BUILD_LINUX)
	pthread_t m_thread;
	sem_t *m_slave_lock;
	char m_memory_name[256], m_slave_lock_name[256];
#endif
public:
	BaseRunner(const char *filename, size_t bank_size, int bank_num, int pid, size_t stack_size)
		: m_bank_size(bank_size), m_bank_num(bank_num), m_stack_size(stack_size)
	{
#if defined(CENTAURUS_BUILD_WINDOWS)
		HANDLE hInputFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

        DWORD dwFileSizeHigh;
        DWORD dwFileSize = GetFileSize(hInputFile, &dwFileSizeHigh);
        m_input_size = ((size_t)dwFileSizeHigh << 32) | dwFileSize;

		HANDLE hInputMapping = CreateFileMappingA(input_file_handle, NULL, PAGE_READONLY, 0, dwFileSize, NULL);

        m_input_window = MapViewOfFile(hInputMapping, FILE_MAP_READ, 0, 0, 0);

        CloseHandle(hInputMapping);
        CloseHandle(hInputFile);

		sprintf_s(m_memory_name, "%s%s[%u]Window", PROGRAM_UUID, PROGRAM_NAME, pid);

		sprintf_s(m_slave_lock_name, "%s%s[%u]SlaveLock", PROGRAM_UUID, PROGRAM_NAME, pid);
#elif defined(CENTAURUS_BUILD_LINUX)
        int fd = open(filename, O_RDONLY);

        struct stat sb;
        fstat(fd, &sb);
        m_input_size = sb.st_size;

        m_input_window = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);

        close(fd);

        snprintf(m_memory_name, sizeof(m_memory_name), "/%s%s[%d]Window", PROGRAM_UUID, PROGRAM_NAME, pid);

		snprintf(m_slave_lock_name, sizeof(m_slave_lock_name), "/%s%s[%d]SlaveLock", PROGRAM_UUID, PROGRAM_NAME, pid);
#endif
	}
	virtual ~BaseRunner()
	{
#if defined(CENTAURUS_BUILD_WINDOWS)
        UnmapViewOfFile(m_input_window);
#elif defined(CENTAURUS_BUILD_LINUX)
        munmap(const_cast<void *>(m_input_window), m_input_size);
#endif
	}
protected:
	size_t get_main_window_size() const
	{
		return ALIGN_NEXT(m_bank_size, IPC_PAGESIZE) * m_bank_num;
	}
	size_t get_sub_window_size() const
	{
		return ALIGN_NEXT(sizeof(WindowBankEntry) * m_bank_num, IPC_PAGESIZE);
	}
	size_t get_window_size() const
	{
		return get_main_window_size() + get_sub_window_size();
	}
    size_t get_input_size() const
    {
        return m_input_size;
    }
    const void *get_input() const
    {
        return m_input_window;
    }
public:
    template<typename F>
    void _start(F runner, void *context)
    {
        clock_t start_time = clock();

#if defined(CENTAURUS_BUILD_WINDOWS)
        HANDLE m_thread = CreateThread(NULL, m_stack_size, runner, context, 0, NULL);
#elif defined(CENTAURUS_BUILD_LINUX)
        pthread_t thread;
        pthread_attr_t attr;

        pthread_attr_init(&attr);

        pthread_attr_setstacksize(&attr, m_stack_size);

        pthread_create(&m_thread, &attr, runner, context);
#endif

        clock_t end_time = clock();

        //char buf[64];
        //snprintf(buf, 64, "Elapsed time = %lf[ms]\r\n", (double)(end_time - start_time) * 1000.0 / CLOCKS_PER_SEC);
        //Logger::WriteMessage(buf);
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
