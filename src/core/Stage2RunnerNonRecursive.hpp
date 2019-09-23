#pragma once

#include "BaseRunner.hpp"

#include <atomic>
#include <vector>
#include <tuple>
#include <cassert>
#include <iostream>
#include <cstring>

namespace Centaurus
{

namespace detail
{

enum struct StackEntryTag : int {
  START_MARKER = -1,
  END_MARKER = 0,
  VALUE = 1,
};

constexpr inline bool isValueTag(StackEntryTag tag)
{
  return static_cast<int>(tag) >= static_cast<int>(detail::StackEntryTag::VALUE);
}

}

class NonRecursiveReductionRunner : public BaseRunner
{
  ReductionListener m_listener = nullptr;
  TransferListener m_xferlistener = nullptr;
  void *m_listener_context;

  std::atomic<int>* reduction_counter;

protected:
  using semantic_value_type = uint64_t;
  NonRecursiveReductionRunner(const char *filename, size_t bank_size, int bank_num, int master_pid, void *context = nullptr, std::atomic<int> *counter = nullptr)
    : BaseRunner(filename, bank_size, bank_num, master_pid), m_listener_context(context), reduction_counter(counter)
  {}

  static void push_start_marker(const CSTMarker& marker, std::vector<CSTMarker>& starts, std::vector<detail::StackEntryTag>& tags)
  {
    tags.emplace_back(detail::StackEntryTag::START_MARKER);
    starts.emplace_back(marker);
  }
  static void push_end_marker(const CSTMarker& marker, std::vector<CSTMarker>& ends, std::vector<detail::StackEntryTag>& tags)
  {
    tags.emplace_back(detail::StackEntryTag::END_MARKER);
    ends.emplace_back(marker);
  }
  static void push_value(const semantic_value_type val, std::vector<semantic_value_type>& values, std::vector<detail::StackEntryTag>& tags)
  {
    if (val == 0) return;
    if (detail::isValueTag(tags.back())) {
      tags.back() = static_cast<detail::StackEntryTag>(static_cast<int>(tags.back()) + 1);
    } else {
      tags.emplace_back(detail::StackEntryTag::VALUE);
    }
#if PYCENTAURUS
    values.front()++;
#else
    values.emplace_back(val);
#endif
  }
  void reduce_by_end_marker(const CSTMarker& marker, std::vector<CSTMarker>& starts, std::vector<semantic_value_type>& values, std::vector<detail::StackEntryTag>& tags)
  {
    assert(marker.get_machine_id() == starts.back().get_machine_id());
    SymbolEntry sym(marker.get_machine_id(), starts.back().get_offset(), marker.get_offset());
    int value_count = static_cast<int>(tags.back());
    if (value_count < 1) {
      value_count = 0;
    } else {
      tags.pop_back();
    }
    assert(tags.empty() || !detail::isValueTag(tags.back()));
#if PYCENTAURUS
    auto new_val = m_listener(&sym, values.data(), value_count, m_listener_context);
#else
    auto new_val = m_listener(&sym, &values[values.size() - value_count], value_count, m_listener_context);
    values.resize(values.size() - value_count);
#endif
    if (reduction_counter != nullptr) {
      (*reduction_counter)++;
    }

    assert(tags.back() == detail::StackEntryTag::START_MARKER);
    tags.pop_back();
    starts.pop_back();
    push_value(new_val, values, tags);
#if PYCENTAURUS
    values.front() = 0;
#endif
  }

  void invoke_transfer_listener(int index, int new_index)
  {
    if (m_xferlistener != nullptr)
      m_xferlistener(index, new_index, m_listener_context);
  }

public:
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

class Stage2Runner : public NonRecursiveReductionRunner
{
  friend BaseRunner;
  int m_current_bank;

private:
  void thread_runner_impl()
  {
    m_current_bank = -1;
    while (true) {
      uint64_t *data = reinterpret_cast<uint64_t*>(acquire_bank());
      if (data == NULL) break;
      reduce_bank(data);
      release_bank();
    }
  }
  void reduce_bank(uint64_t *src)
  {
    auto ptr = new std::tuple<std::vector<CSTMarker>,
                              std::vector<CSTMarker>,
                              std::vector<semantic_value_type>,
                              std::vector<detail::StackEntryTag>>;
    auto& starts = std::get<0>(*ptr);
    auto& ends   = std::get<1>(*ptr);
    auto& values = std::get<2>(*ptr);
    auto& tags   = std::get<3>(*ptr);
#if PYCENTAURUS
    values.emplace_back(0);
#endif
    for (int i = 0; i < m_bank_size / 8; i++) {
      if (src[i] == 0) break;
      CSTMarker marker(src[i]);
      if (marker.is_start_marker()) {
        push_start_marker(marker, starts, tags);
      } else if (starts.empty()) {
        assert(marker.is_end_marker());
        push_end_marker(marker, ends, tags);
      } else {
        assert(marker.is_end_marker());
        reduce_by_end_marker(marker, starts, values, tags);
      }
    }
#if PYCENTAURUS
    auto it = src;
    *it++ = starts.size();
    std::memcpy(it, starts.data(), starts.size()*sizeof(CSTMarker));
    it += starts.size();
    *it++ = ends.size();
    std::memcpy(it, ends.data(), ends.size()*sizeof(CSTMarker));
    it += ends.size();
    *it++ = values.size();
    std::memcpy(it, values.data(), values.size()*sizeof(semantic_value_type));
    it += values.size();
    *it++ = tags.size();
    std::memcpy(it, tags.data(), tags.size()*sizeof(detail::StackEntryTag));
    assert(reinterpret_cast<uint64_t*>(reinterpret_cast<detail::StackEntryTag*>(it) + tags.size()) < src + m_bank_size / 8);
#else
    *src = reinterpret_cast<uint64_t>(ptr);
#endif
  }

  void *acquire_bank()
  {
    wait_on_semaphore();
    WindowBankEntry *banks = reinterpret_cast<WindowBankEntry *>(m_sub_window);
    while (true) {
      for (int i = 0; i < m_bank_num; i++) {
        WindowBankState old_state = WindowBankState::Stage1_Unlocked;
        if (banks[i].state.compare_exchange_weak(old_state, WindowBankState::Stage2_Locked)) {
          invoke_transfer_listener(-1, banks[i].number);
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
    invoke_transfer_listener(m_current_bank, -1);
    banks[m_current_bank].state.store(WindowBankState::Stage2_Unlocked);
    m_current_bank = -1;
  }

public:
  Stage2Runner(const char *filename, size_t bank_size, int bank_num, int master_pid, void *context = nullptr, std::atomic<int> *counter = nullptr)
    : NonRecursiveReductionRunner(filename, bank_size, bank_num, master_pid, context, counter)
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
};
}
