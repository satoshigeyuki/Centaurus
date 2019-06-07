#pragma once

#include "BaseRunner.hpp"
#include "CodeGenInterface.hpp"

namespace Centaurus
{
class Stage1Runner : public BaseRunner
{
  friend BaseRunner;
    IParser *m_parser;
    int m_current_bank, m_counter;
	const void *m_result;
private:
	void *acquire_bank();
	void release_bank();
	void reset_banks();
	void signal_exit();
public:
	Stage1Runner(const char *filename, IParser *parser, size_t bank_size, int bank_num);
	virtual ~Stage1Runner();
    virtual void start() override
    {
      _start<Stage1Runner>();
    }
    virtual void *feed_callback() override
    {
        release_bank();
        return acquire_bank();
    }
	const void *get_result() const
	{
		return m_result;
	}

private:
  void thread_runner_impl();
};
}
