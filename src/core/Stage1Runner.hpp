#pragma once

#include "BaseRunner.hpp"
#include "CodeGenInterface.hpp"
#include "PtrRange.hpp"

#include <cstring>
#include <vector>

namespace Centaurus
{
class Stage1Runner : public BaseRunner
{
  friend BaseRunner;
  IParser *m_parser;
  int m_current_bank, m_counter;
  const void *m_result;
  const bool is_dry;
  const bool is_result_captured;
  std::vector<detail::ConstPtrRange<CSTMarker>> result_chunks_;

private:
  void thread_runner_impl()
  {
    m_current_bank = -1;
    m_counter = 0;
    reset_banks();

    m_result = (*m_parser)(static_cast<BaseListener*>(this), m_input_window);

    release_bank();
    signal_exit();
  }
  void *acquire_bank()
  {
    WindowBankEntry *banks = (WindowBankEntry *)m_sub_window;
    while (true) {
      for (int i = 0; i < m_bank_num; i++) {
        WindowBankState old_state = WindowBankState::Free;
        if (banks[i].state.compare_exchange_weak(old_state, WindowBankState::Stage1_Locked)) {
          m_current_bank = i;
          return (char *)m_main_window + m_bank_size * i;
        }
      }
    }
  }
  void release_bank()
  {
    if (m_current_bank != -1) {
      if (is_result_captured) {
        auto chunk_ptr = new char[m_bank_size];
        std::memcpy(chunk_ptr, static_cast<char*>(m_main_window) + m_bank_size * m_current_bank, m_bank_size);
        result_chunks_.emplace_back(reinterpret_cast<CSTMarker*>(chunk_ptr), m_bank_size / 8);
      }
      WindowBankEntry *banks = (WindowBankEntry *)m_sub_window;
      banks[m_current_bank].number = m_counter++;
      if (is_dry) {
        banks[m_current_bank].state.store(WindowBankState::Free); // skip Stages 2 & 3
      } else {
        banks[m_current_bank].state.store(WindowBankState::Stage1_Unlocked);
      }
      m_current_bank = -1;
      release_semaphore();
    }
  }
  void reset_banks()
  {
    WindowBankEntry *banks = (WindowBankEntry *)m_sub_window;
    for (int i = 0; i < m_bank_num; i++) {
      banks[i].state.store(WindowBankState::Free);
    }
  }
  void signal_exit()
  {
    WindowBankEntry *banks = (WindowBankEntry *)m_sub_window;
    while (true) {
      int count = 0;
      for (int i = 0; i < m_bank_num; i++) {
        WindowBankState state = banks[i].state.load();
        if (state != WindowBankState::Free)
          count++;
      }
      if (count == 0) break;
    }
    for (int i = 0; i < m_bank_num; i++) {
      banks[i].state.store(WindowBankState::YouAreDone);
      release_semaphore();
    }
  }

public:
  Stage1Runner(const char *filename, IParser *parser, size_t bank_size, int bank_num, bool is_dry=false, bool is_result_captured=false)
    : BaseRunner(filename, bank_size, bank_num), m_parser(parser), is_dry(is_dry), is_result_captured(is_result_captured)
  {
    acquire_memory(true);
    create_semaphore();
  }
  virtual ~Stage1Runner() {
    close_semaphore(true);
    release_memory(true);
  }

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
  const std::vector<detail::ConstPtrRange<CSTMarker>>& result_chunks() const
  {
    return result_chunks_;
  }
};
}
