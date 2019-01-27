#pragma once

#include <vector>
#include <stdint.h>
#include <time.h>
#include "BaseRunner.hpp"
#include "CodeGenEM64T.hpp"

namespace Centaurus
{
class Stage2Runner : public BaseRunner
{
    void *m_listener_context;
    IChaser *m_chaser;
    size_t m_bank_size;
	int m_current_bank;
    const uint64_t *m_sv_list;
	std::vector<SymbolEntry> m_sym_stack;
private:
#if defined(CENTAURUS_BUILD_WINDOWS)
	static DWORD WINAPI thread_runner(LPVOID param);
#elif defined(CENTAURUS_BUILD_LINUX)
	static void *thread_runner(void *param);
#endif
	void reduce_bank(uint64_t *ast);
	int parse_subtree(uint64_t *ast, int position);
	void *acquire_bank();
	void release_bank();
public:
	Stage2Runner(const char *filename, IChaser *chaser, size_t bank_size, int bank_num, int master_pid, void *context = nullptr);
	virtual ~Stage2Runner();
	virtual void start() override
	{
        _start(Stage2Runner::thread_runner, this);
    }
    virtual void terminal_callback(int id, const void *start, const void *end) override
    {
		long start_offset = (const char *)start - (const char *)m_input_window;
		long end_offset = (const char *)end - (const char *)m_input_window;
		//m_sym_stack.emplace_back(-id, start_offset, end_offset);
    }
    virtual const void *nonterminal_callback(int id, const void *input) override
    {
		long start_offset = (const char *)input - (const char *)m_input_window;
		long end_offset = m_sv_list[0] & 0xFFFFFFFFFFFFul;
		m_sym_stack.emplace_back(id, start_offset, end_offset, m_sv_list[1]);
		m_sv_list += 2;
        return (const char *)m_input_window + end_offset;
    }
};
}
