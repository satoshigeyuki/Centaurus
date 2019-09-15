#pragma once

#include <vector>
#include <atomic>
#include <stdint.h>
#include "BaseRunner.hpp"

namespace Centaurus
{
class Stage2Runner : public BaseRunner
{
  friend BaseRunner;
  ReductionListener m_listener;
  TransferListener m_xferlistener;
  void *m_listener_context;
  int m_current_bank;
  std::vector<uint64_t> val_stack;

  std::atomic<int>* reduction_counter;

private:
  void thread_runner_impl()
  {
    m_current_bank = -1;
    val_stack.clear();
    while (true) {
      uint64_t *data = reinterpret_cast<uint64_t*>(acquire_bank());
      if (data == NULL) break;
      reduce_bank(data);
      release_bank();
    }
  }
  void reduce_bank(uint64_t *src)
  {
    for (int i = 0; i < m_bank_size / 8; i++) {
      CSTMarker marker(src[i]);
      if (marker.is_start_marker()) {
        i = parse_subtree(src, i);
      } else if (src[i] == 0) {
        break;
      }
    }
  }
  int parse_subtree(uint64_t *ast, int position)
  {
    CSTMarker start_marker(ast[position]);
    int i, j;
    j = position + 1;
    for (i = position + 1; i < m_bank_size / 8; i++) {
      if (ast[i] == 0) {
        throw SimpleException("Null entry in CST window.");
      }
      CSTMarker marker(ast[i]);
      if (marker.is_start_marker()) {
        int k = parse_subtree(ast, i);
        if (k < m_bank_size / 8) {
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
      } else if (marker.is_end_marker()) {
        val_stack.clear();
        for (int l = position + 1; l < j; l += 2) {
          val_stack.emplace_back(ast[l + 1]);
        }
        long tag = 0;
        SymbolEntry sym(marker.get_machine_id(), start_marker.get_offset(), marker.get_offset());
        if (m_listener != nullptr)
          tag = m_listener(&sym, val_stack.data(), val_stack.size(), m_listener_context);
        if (reduction_counter != nullptr) {
          (*reduction_counter)++;
        }
        //Zero-fill the SV list
        for (int k = position + 1; k < j; k++) {
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

  void *acquire_bank()
  {
    wait_on_semaphore();
    WindowBankEntry *banks = reinterpret_cast<WindowBankEntry *>(m_sub_window);
    while (true) {
      for (int i = 0; i < m_bank_num; i++) {
        WindowBankState old_state = WindowBankState::Stage1_Unlocked;
        if (banks[i].state.compare_exchange_weak(old_state, WindowBankState::Stage2_Locked)) {
          if (m_xferlistener != nullptr)
            m_xferlistener(-1, banks[i].number, m_listener_context);
          m_current_bank = i;
          return (char *)m_main_window + m_bank_size * i;
        } else {
          if (old_state == WindowBankState::YouAreDone) {
            return NULL;
          }
        }
      }
    }
  }
  void release_bank()
  {
    WindowBankEntry *banks = reinterpret_cast<WindowBankEntry *>(m_sub_window);
    if (m_xferlistener != nullptr)
      m_xferlistener(m_current_bank, -1, m_listener_context);
    banks[m_current_bank].state.store(WindowBankState::Stage2_Unlocked);
    m_current_bank = -1;
  }

public:
  Stage2Runner(const char *filename, size_t bank_size, int bank_num, int master_pid, void *context = nullptr, std::atomic<int> *counter = nullptr)
    : BaseRunner(filename, bank_size, bank_num, master_pid), m_listener_context(context), reduction_counter(counter)
  {
    acquire_memory(false);
    open_semaphore();
  }
  virtual ~Stage2Runner()
  {
    close_semaphore(false);
    release_memory(false);
  }
  virtual void start() override
  {
    _start<Stage2Runner>();
  }

  virtual void register_python_listener(ReductionListener listener, TransferListener xferlistener) override
  {
    m_listener = listener;
    m_xferlistener = xferlistener;
  }
  void register_listener(ReductionListener listener)
  {
    m_listener = listener;
    m_xferlistener = nullptr;
  }
};
}
