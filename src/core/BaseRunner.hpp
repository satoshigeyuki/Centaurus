#pragma once

#include <stdio.h>
#include <atomic>

#include "BaseListener.hpp"
#include "Platform.hpp"

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
    bool get_sign_bit() const
    {
        return m_value & ((uint64_t)1 << 63);
    }
	bool is_start_marker() const
	{
		return get_sign_bit() && get_machine_id() != 0;
	}
	bool is_end_marker() const
	{
		return !get_sign_bit() && get_machine_id() != 0;
	}
    bool is_sv_marker() const
    {
        return get_sign_bit() && get_machine_id() == 0;
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
class SVCapsule
{
    int m_mid;
    const void *m_next_ptr;
    uint64_t m_tag;
public:
    SVCapsule(const void *window, uint64_t v0, uint64_t v1)
    {
        CSTMarker marker(v0);

        m_mid = marker.get_machine_id();
        m_next_ptr = marker.offset_ptr(window);
        m_tag = v1;
    }
    SVCapsule()
        : m_mid(0), m_next_ptr(NULL), m_tag(0)
    {

    }
    const void *get_next_ptr() const
    {
        return m_next_ptr;
    }
	uint64_t get_tag() const
	{
		return m_tag;
	}
    int get_machine_id() const
    {
        return m_mid;
    }
};

typedef long (CENTAURUS_CALLBACK * ReductionListener)(const SymbolEntry *symbol, uint64_t *values, int num_values, void *context);
typedef void (CENTAURUS_CALLBACK * TransferListener)(int index, int new_index, void *context);

class BaseRunner : public BaseListener
{
    static constexpr size_t STACK_SIZE = 1024 * 1024 * 1024;
protected:
	enum class WindowBankState
	{
		Free,
		Stage1_Locked,
		Stage1_Unlocked,
		Stage2_Locked,
		Stage2_Unlocked,
		Stage3_Locked,
        YouAreDone
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
#if defined(CENTAURUS_BUILD_WINDOWS)
    HANDLE m_mem_handle;
    HANDLE m_slave_lock;
	HANDLE m_thread;
	char m_memory_name[MAX_PATH], m_slave_lock_name[MAX_PATH];
#elif defined(CENTAURUS_BUILD_LINUX)
	pthread_t m_thread;
	sem_t *m_slave_lock;
	char m_memory_name[256], m_slave_lock_name[256];
#endif
public:
	BaseRunner(const char *filename, size_t bank_size, int bank_num, int pid = get_current_pid())
		: m_bank_size(bank_size), m_bank_num(bank_num)
	{
#if defined(CENTAURUS_BUILD_WINDOWS)
		HANDLE hInputFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

        DWORD dwFileSizeHigh;
        DWORD dwFileSize = GetFileSize(hInputFile, &dwFileSizeHigh);
        m_input_size = ((size_t)dwFileSizeHigh << 32) | dwFileSize;

		HANDLE hInputMapping = CreateFileMappingA(hInputFile, NULL, PAGE_READONLY, dwFileSizeHigh, dwFileSize, NULL);

        m_input_window = MapViewOfFile(hInputMapping, FILE_MAP_READ, 0, 0, 0);

        //volatile char a = *((const char *)m_input_window + (m_input_size - 1));

        //CloseHandle(hInputMapping);
        //CloseHandle(hInputFile);

		sprintf(m_memory_name, "%s%s[%u]Window", PROGRAM_UUID, PROGRAM_NAME, pid);

		sprintf(m_slave_lock_name, "%s%s[%u]SlaveLock", PROGRAM_UUID, PROGRAM_NAME, pid);
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
public:
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
	virtual void start() = 0;
    template<typename RunnerImpl>
    void _start()
    {
        clock_t start_time = clock();

#if defined(CENTAURUS_BUILD_WINDOWS)
        m_thread = CreateThread(NULL, STACK_SIZE, BaseRunner::thread_runner<RunnerImpl>, this, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);
#elif defined(CENTAURUS_BUILD_LINUX)
        pthread_t thread;
        pthread_attr_t attr;

        pthread_attr_init(&attr);

        pthread_attr_setstacksize(&attr, STACK_SIZE);

        pthread_create(&m_thread, &attr, BaseRunner::thread_runner<RunnerImpl>, this);
#endif

        clock_t end_time = clock();

        //char buf[64];
        //snprintf(buf, 64, "Elapsed time = %lf[ms]\r\n", (double)(end_time - start_time) * 1000.0 / CLOCKS_PER_SEC);
        //Logger::WriteMessage(buf);
    }
	virtual void wait()
	{
#if defined(CENTAURUS_BUILD_WINDOWS)
		WaitForSingleObject(m_thread, INFINITE);
#elif defined(CENTAURUS_BUILD_LINUX)
		pthread_join(m_thread, NULL);
#endif
	}

  virtual void register_python_listener(ReductionListener listener, TransferListener xferlistener) {}

private:
  template <typename RunnerImpl>
#if defined(CENTAURUS_BUILD_WINDOWS)
  static DWORD WINAPI thread_runner(LPVOID param)
#elif defined(CENTAURUS_BUILD_LINUX)
  static void *thread_runner(void *param)
#endif
  {
    reinterpret_cast<RunnerImpl*>(param)->thread_runner_impl();
#if defined(CENTAURUS_BUILD_WINDOWS)
    ExitThread(0);
#elif defined(CENTAURUS_BUILD_LINUX)
    return nullptr;
#endif
  }

protected:
  void acquire_memory(bool is_allocating)
  {
#if defined(CENTAURUS_BUILD_WINDOWS)
    m_mem_handle  = (is_allocating) ? CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, get_window_size(), m_memory_name)
                                    : OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, m_memory_name);
    m_sub_window  = MapViewOfFile(m_mem_handle, FILE_MAP_ALL_ACCESS, 0, 0, get_sub_window_size());
    m_main_window = MapViewOfFile(m_mem_handle, FILE_MAP_ALL_ACCESS, 0, get_sub_window_size(), get_main_window_size());
#elif defined(CENTAURUS_BUILD_LINUX)
    int fd = shm_open(m_memory_name, (is_allocating) ? O_RDWR | O_CREAT : O_RDWR, 0600);
    ftruncate(fd, get_window_size());
    m_sub_window  = mmap(NULL, get_sub_window_size(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    m_main_window = mmap(NULL, get_main_window_size(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, get_sub_window_size());
    if (is_allocating) close(fd);
#endif
  }
  void release_memory(bool is_finalizing)
  {
#if defined(CENTAURUS_BUILD_WINDOWS)
    if (m_main_window != NULL) UnmapViewOfFile(m_main_window);
    if (m_sub_window != NULL)  UnmapViewOfFile(m_sub_window);
    if (m_mem_handle != NULL)  CloseHandle(m_mem_handle);
#elif defined(CENTAURUS_BUILD_LINUX)
    if (m_main_window != MAP_FAILED) munmap(m_main_window, get_main_window_size());
    if (m_sub_window != MAP_FAILED)  munmap(m_sub_window, get_sub_window_size());
    if (is_finalizing) {
      shm_unlink(m_memory_name);
    }
#endif
  }
  void create_semaphore()
  {
#if defined(CENTAURUS_BUILD_WINDOWS)
    m_slave_lock = CreateSemaphoreA(NULL, 0, m_bank_num, m_slave_lock_name);
#elif defined(CENTAURUS_BUILD_LINUX)
    m_slave_lock = sem_open(m_slave_lock_name, O_CREAT | O_EXCL, 0600, 0);
#endif
  }
  void open_semaphore()
  {
#if defined(CENTAURUS_BUILD_WINDOWS)
    m_slave_lock = OpenSemaphoreA(SYNCHRONIZE, FALSE, m_slave_lock_name);
#elif defined(CENTAURUS_BUILD_LINUX)
    m_slave_lock = sem_open(m_slave_lock_name, 0);
#endif
  }
  void release_semaphore() {
#if defined(CENTAURUS_BUILD_WINDOWS)
    ReleaseSemaphore(m_slave_lock, 1, NULL);
#elif defined(CENTAURUS_BUILD_LINUX)
    sem_post(m_slave_lock);
#endif
  }
  void close_semaphore(bool is_finalizing) {
#if defined(CENTAURUS_BUILD_WINDOWS)
    if (m_slave_lock != NULL) CloseHandle(m_slave_lock);
#elif defined(CENTAURUS_BUILD_LINUX)
    if (m_slave_lock != SEM_FAILED) sem_close(m_slave_lock);
    if (is_finalizing) sem_unlink(m_slave_lock_name);
#endif
  }
  void wait_on_semaphore() {
#if defined(CENTAURUS_BUILD_WINDOWS)
    WaitForSingleObject(m_slave_lock, INFINITE);
#elif defined(CENTAURUS_BUILD_LINUX)
    sem_wait(m_slave_lock);
#endif
  }

};
}
