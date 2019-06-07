#include "Stage3Runner.hpp"

namespace Centaurus
{
void Stage3Runner::thread_runner_impl()
{
	m_current_bank = -1;
	m_counter = 0;
	m_current_window = NULL;
	m_window_position = 0;
	m_sym_stack.clear();

	reduce();

	release_bank();
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
#ifdef CHASER_ENABLED
				const void *chaser_result = (*m_chaser)[start_marker.get_machine_id()](this, start_marker.offset_ptr(m_input_window));
				if (m_sv_index != values.size())
				{
					std::cerr << "SV list undigested: " << m_sv_index << "/" << values.size() << "." << std::endl;
				}
				if (chaser_result != marker.offset_ptr(m_input_window))
				{
					std::cerr << "Chaser aborted: " << std::hex << (uint64_t)chaser_result << "/" << (uint64_t)marker.offset_ptr(m_input_window) << std::dec << std::endl;
				}
#else
				for (int l = 0; l < values.size(); l++)
				{
					m_sym_stack.emplace_back(values[l].get_machine_id(), 0, 0, values[l].get_tag());
				}
#endif
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
	: BaseRunner(filename, bank_size, bank_num, master_pid), m_chaser(chaser), m_listener_context(context)
{
  acquire_memory(false);
}
Stage3Runner::~Stage3Runner()
{
  release_memory(false);
}
}
