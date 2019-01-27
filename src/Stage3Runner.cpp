#include "Stage3Runner.hpp"

namespace Centaurus
{
#if defined(CENTAURUS_BUILD_WINDOWS)
DWORD WINAPI Stage3Runner::thread_runner(LPVOID param)
#elif defined(CENTAURUS_BUILD_LINUX)
void *Stage3Runner::thread_runner(void *param)
#endif
{
	Stage3Runner *instance = reinterpret_cast<Stage3Runner *>(param);

	instance->m_current_bank = -1;
	instance->m_counter = 0;
	instance->m_current_window = NULL;
	instance->m_window_position = 0;
	instance->m_sym_stack.clear();

	instance->reduce();

	instance->release_bank();

#if defined(CENTAURUS_BUILD_WINDOWS)
	ExitThread(0);
#elif defined(CENTAURUS_BUILD_LINUX)
	return NULL;
#endif
}
SVCapsule Stage3Runner::reduce()
{
	std::vector<SVCapsule> values;

	if (m_current_window == NULL || m_window_position >= m_bank_size / 8)
	{
		release_bank();

		m_current_window = reinterpret_cast<uint64_t *>(acquire_bank());

		if (m_current_window == NULL) return SVCapsule();

		m_window_position = 0;
	}

	CSTMarker start_marker(m_current_window[m_window_position++]);

	while (true)
	{
		for (; m_window_position < m_bank_size / 8; m_window_position++)
		{
			CSTMarker marker(m_current_window[m_window_position]);

			if (marker.is_start_marker())
			{
				values.push_back(reduce());
			}
			else if (marker.is_sv_marker())
			{
				uint64_t v0 = m_current_window[m_window_position];
				uint64_t v1 = m_current_window[m_window_position + 1];
				values.emplace_back(m_input_window, v0, v1);
				m_window_position++;
			}
			else if (marker.is_end_marker())
			{
				m_sym_stack.clear();
				m_sym_stack.emplace_back(start_marker.get_machine_id(), start_marker.get_offset(), marker.get_offset());
				m_sv_index = 0;
				m_sv_list = &values;
				const void *chaser_result = (*m_chaser)[start_marker.get_machine_id()](this, start_marker.offset_ptr(m_input_window));
				if (m_sv_index != values.size())
				{
					std::cerr << "SV list undigested: " << m_sv_index << "/" << values.size() << "." << std::endl;
				}
				if (chaser_result != marker.offset_ptr(m_input_window))
				{
					std::cerr << "Chaser aborted: " << std::hex << (uint64_t)chaser_result << "/" << (uint64_t)marker.offset_ptr(m_input_window) << std::dec << std::endl;
				}
				long tag = 0;
				if (m_listener != nullptr)
					tag = m_listener(m_sym_stack.data(), m_sym_stack.size(), m_listener_context);
				return SVCapsule(m_input_window, marker.get_offset(), tag);
			}
		}
		release_bank();
		m_current_window = reinterpret_cast<uint64_t *>(acquire_bank());
		if (m_current_window == NULL) return SVCapsule();
		m_window_position = 0;
	}
}
void *Stage3Runner::acquire_bank()
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

					if (m_xferlistener != nullptr)
						m_xferlistener(i, banks[i].number, m_listener_context);

					//std::cout << "Bank " << banks[i].number << " reached Stage3" << std::endl;

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
void Stage3Runner::release_bank()
{
	if (m_current_bank != -1)
	{
		WindowBankEntry *banks = reinterpret_cast<WindowBankEntry *>(m_sub_window);

		banks[m_current_bank].state.store(WindowBankState::Free);

		m_current_bank = -1;
	}
}
Stage3Runner::Stage3Runner(const char *filename, IChaser *chaser, size_t bank_size, int bank_num, int master_pid, void *context)
	: BaseRunner(filename, bank_size, bank_num, master_pid), m_chaser(chaser), m_bank_size(bank_size), m_listener_context(context)
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
Stage3Runner::~Stage3Runner()
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
}
