#pragma once

#include "BaseRunner.hpp"
#include "CodeGenInterface.hpp"
#include "CodeGenEM64T.hpp"

namespace Centaurus
{
class Stage3Runner : public BaseRunner
{
    IChaser *m_chaser;
    size_t m_bank_size;
    int m_current_bank;
    int m_counter;
    const uint64_t *m_current_window;
    int m_window_position;
    int m_sv_index;
    const std::vector<SVCapsule> *m_sv_list;
	std::vector<SymbolEntry> m_sym_stack;
private:
#if defined(CENTAURUS_BUILD_WINDOWS)
	static DWORD WINAPI thread_runner(LPVOID param);
#elif defined(CENTAURUS_BUILD_LINUX)
	static void *thread_runner(void *param);
#endif
	SVCapsule reduce();
	void *acquire_bank();
	void release_bank();
public:
	Stage3Runner(const char *filename, IChaser *chaser, size_t bank_size, int bank_num, int master_pid);
	virtual ~Stage3Runner();
	virtual void start() override
	{
        _start(Stage3Runner::thread_runner, this);
	}
    virtual void terminal_callback(int id, const void *start, const void *end) override
    {
		long start_offset = (const char *)start - (const char *)m_input_window;
		long end_offset = (const char *)end - (const char *)m_input_window;
		//m_sym_stack.emplace_back(-id, start_offset, end_offset);
    }
    virtual const void *nonterminal_callback(int id, const void *input) override
    {
        if (m_sv_index < m_sv_list->size())
        {
            SVCapsule sv = m_sv_list->at(m_sv_index);
            m_sv_index++;
			long start_offset = (const char *)input - (const char *)m_input_window;
			long end_offset = (const char *)sv.get_next_ptr() - (const char *)m_input_window;
			m_sym_stack.emplace_back(id, start_offset, end_offset, sv.get_tag());
            return sv.get_next_ptr();
        }
        return NULL;
    }
};
}
