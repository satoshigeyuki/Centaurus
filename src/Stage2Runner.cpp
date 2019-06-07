#include "Stage2Runner.hpp"

namespace Centaurus
{
std::atomic<int> Stage2Runner::s_token_count;
std::atomic<int> Stage2Runner::s_nonterminal_count;

void Stage2Runner::thread_runner_impl()
{
	m_sv_list = NULL;
	m_current_bank = -1;
	m_sym_stack.clear();

	while (true)
	{
		uint64_t *data = reinterpret_cast<uint64_t*>(acquire_bank());

		if (data == NULL) break;

		reduce_bank(data);

		release_bank();
	}
}
void Stage2Runner::reduce_bank(uint64_t *src)
{
	for (int i = 0; i < m_bank_size / 8; i++)
	{
		CSTMarker marker(src[i]);
		if (marker.is_start_marker())
		{
			i = parse_subtree(src, i);
		}
		else if (src[i] == 0)
		{
			break;
		}
	}
}
int Stage2Runner::parse_subtree(uint64_t *ast, int position)
{
	CSTMarker start_marker(ast[position]);
	int i, j;
	j = position + 1;
	for (i = position + 1; i < m_bank_size / 8; i++)
	{
		if (ast[i] == 0)
		{
			throw SimpleException("Null entry in CST window.");
		}
		CSTMarker marker(ast[i]);
		if (marker.is_start_marker())
		{
			int k = parse_subtree(ast, i);
			if (k < m_bank_size / 8)
			{
				uint64_t subtree_sv[2];
				subtree_sv[0] = ast[i];
				ast[i] = 0;
				ast[j] = subtree_sv[0];
				subtree_sv[1] = ast[i + 1];
				ast[i + 1] = 0;
				ast[j + 1] = subtree_sv[1];
				j += 2;
			}
			i = k;
		}
		else if (marker.is_end_marker())
		{
			m_sym_stack.clear();
			m_sym_stack.emplace_back(marker.get_machine_id(), start_marker.get_offset(), marker.get_offset());
			m_sv_list = &ast[position + 1];
#ifdef CHASER_ENABLED
			const void *chaser_result = (*m_chaser)[marker.get_machine_id()](this, start_marker.offset_ptr(m_input_window));
			if (m_sv_list - &ast[position + 1] < j - position - 1)
			{
				std::cerr << "SV list undigested: " << (m_sv_list - &ast[position + 1]) / 2 << "/" << (j - position - 1) / 2 << "." << std::endl;
			}
			if (chaser_result != marker.offset_ptr(m_input_window))
			{
				std::cerr << "Chaser aborted: " << std::hex << (uint64_t)chaser_result << "/" << (uint64_t)marker.offset_ptr(m_input_window) << std::dec << std::endl;
			}
#else
			for (int l = position + 1; l < j; l += 2)
			{
				CSTMarker sv_marker(ast[l]);
				m_sym_stack.emplace_back(sv_marker.get_machine_id(), 0, 0, ast[l + 1]);
			}
#endif
			long tag = 0;
			if (m_listener != nullptr)
				tag = m_listener(m_sym_stack.data(), m_sym_stack.size(), m_listener_context);

			//Zero-fill the SV list
			for (int k = position + 1; k < j; k++)
			{
				ast[k] = 0;
			}
			//Zero-fill the end marker
			ast[i] = 0;
			ast[position] = ((uint64_t)1 << 63) | marker.get_offset();
			ast[position + 1] = tag;
			return i;
		}
	}
	return i;
}
void *Stage2Runner::acquire_bank()
{
  wait_on_semaphore();
	WindowBankEntry *banks = reinterpret_cast<WindowBankEntry *>(m_sub_window);

	while (true)
	{
		for (int i = 0; i < m_bank_num; i++)
		{
			WindowBankState old_state = WindowBankState::Stage1_Unlocked;

			if (banks[i].state.compare_exchange_weak(old_state, WindowBankState::Stage2_Locked))
			{
				if (m_xferlistener != nullptr)
					m_xferlistener(-1, banks[i].number, m_listener_context);
				m_current_bank = i;
				return (char *)m_main_window + m_bank_size * i;
			}
			else
			{
				if (old_state == WindowBankState::YouAreDone)
				{
					return NULL;
				}
			}
		}
	}
}
void Stage2Runner::release_bank()
{
	WindowBankEntry *banks = reinterpret_cast<WindowBankEntry *>(m_sub_window);

	if (m_xferlistener != nullptr)
		m_xferlistener(m_current_bank, -1, m_listener_context);

	banks[m_current_bank].state.store(WindowBankState::Stage2_Unlocked);

	m_current_bank = -1;

    s_nonterminal_count += m_bank_size / 16;
}
Stage2Runner::Stage2Runner(const char *filename, IChaser *chaser, size_t bank_size, int bank_num, int master_pid, void *context)
	: BaseRunner(filename, bank_size, bank_num, master_pid), m_chaser(chaser), m_listener_context(context)
{
  acquire_memory(false);
  open_semaphore();
}
Stage2Runner::~Stage2Runner()
{
  close_semaphore(false);
  release_memory(false);
}
}
