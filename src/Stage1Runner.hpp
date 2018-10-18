#pragma once

#include "BaseRunner.hpp"
#include "CodeGenInterface.hpp"

namespace Centaurus
{
class Stage1Runner : public BaseRunner
{
    IParser *m_parser;
    int m_current_bank, m_counter;
private:
#if defined(CENTAURUS_BUILD_WINDOWS)
	static DWORD WINAPI thread_runner(LPVOID param);
#elif defined(CENTAURUS_BUILD_LINUX)
	static void *thread_runner(void *param);
#endif
	void *acquire_bank();
	void release_bank();
	void reset_banks();
	void signal_exit();
public:
	Stage1Runner(const char *filename, IParser *parser, size_t bank_size, int bank_num);
	virtual ~Stage1Runner();
    virtual void start() override
    {
        _start(Stage1Runner::thread_runner, static_cast<void *>(this));
    }
    virtual void *feed_callback() override
    {
        release_bank();
        return acquire_bank();
    }
};
}
