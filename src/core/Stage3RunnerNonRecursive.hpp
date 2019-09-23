#pragma once

#include "BaseRunner.hpp"
#include "Stage2RunnerNonRecursive.hpp"
#include "PtrRange.hpp"

namespace Centaurus
{

class Stage3Runner : public NonRecursiveReductionRunner
{
  friend BaseRunner;
  int m_current_bank;
  int m_counter;
  const uint64_t *m_current_window;
  int m_window_position;

private:
  void thread_runner_impl()
  {
    m_current_bank = -1;
    m_counter = 0;
    m_current_window = NULL;
    m_window_position = 0;

    reduce();

    release_bank();
  }

  semantic_value_type reduce()
  {
#if PYCENTAURUS
    auto bank_ptr = reinterpret_cast<uint64_t*>(acquire_bank());
    assert(bank_ptr != nullptr);
    std::vector<CSTMarker> starts(*bank_ptr++, CSTMarker(0));
    std::memcpy(starts.data(), bank_ptr, starts.size()*sizeof(CSTMarker));
    bank_ptr += starts.size();
    std::vector<CSTMarker> ends(*bank_ptr++, CSTMarker(0));
    assert(ends.empty());
    std::vector<semantic_value_type> values(*bank_ptr++);
    std::memcpy(values.data(), bank_ptr, values.size()*sizeof(semantic_value_type));
    assert(values[0] == 0 && values.size() == 1);
    bank_ptr += values.size();
    std::vector<detail::StackEntryTag> tags(*bank_ptr++);
    std::memcpy(tags.data(), bank_ptr, tags.size()*sizeof(detail::StackEntryTag));
#else
    auto bank_base_ptr = reinterpret_cast<std::tuple<std::vector<CSTMarker>,
                                                     std::vector<CSTMarker>,
                                                     std::vector<semantic_value_type>,
                                                     std::vector<detail::StackEntryTag>>**>(acquire_bank());
    assert(bank_base_ptr != nullptr);
    auto& stack_tuple_base = **bank_base_ptr;
    auto& starts = std::get<0>(stack_tuple_base);
    auto& ends   = std::get<1>(stack_tuple_base);
    auto& values = std::get<2>(stack_tuple_base);
    auto& tags   = std::get<3>(stack_tuple_base);
#endif
    release_bank();

    void *current_bank;
    while ((current_bank = acquire_bank()) != nullptr) {
#if PYCENTAURUS
      auto bank_ptr = reinterpret_cast<uint64_t*>(current_bank);
      detail::ConstPtrRange<CSTMarker> starts_next(reinterpret_cast<CSTMarker*>(bank_ptr + 1), bank_ptr[0]);
      bank_ptr += starts_next.size() + 1;
      detail::ConstPtrRange<CSTMarker> ends_next(reinterpret_cast<CSTMarker*>(bank_ptr + 1), bank_ptr[0]);
      bank_ptr += ends_next.size() + 1;
      detail::ConstPtrRange<semantic_value_type> values_next(bank_ptr + 1, bank_ptr[0]);
      bank_ptr += values_next.size() + 1;
      detail::ConstPtrRange<detail::StackEntryTag> tags_next(reinterpret_cast<detail::StackEntryTag*>(bank_ptr + 1), bank_ptr[0]);
#else
      auto& stack_tuple = **reinterpret_cast<std::decay<decltype(stack_tuple_base)>::type**>(current_bank);
      auto& starts_next = std::get<0>(stack_tuple);
      auto& ends_next   = std::get<1>(stack_tuple);
      auto& values_next = std::get<2>(stack_tuple);
      auto& tags_next   = std::get<3>(stack_tuple);
#endif
      auto starts_next_it = starts_next.begin();
      auto ends_next_it   = ends_next.begin();
      auto values_next_it = values_next.begin();
      for (auto cur_tag : tags_next) {
        switch (cur_tag) {
        case detail::StackEntryTag::START_MARKER:
          push_start_marker(*starts_next_it++, starts, tags);
          break;
        case detail::StackEntryTag::VALUE:
#if PYCENTAURUS
          push_value(1, values, tags);
#else
          push_value(*values_next_it++, values, tags);
#endif
          break;
        case detail::StackEntryTag::END_MARKER: {
          assert(!starts.empty());
          reduce_by_end_marker(*ends_next_it++, starts, values, tags);
        } break;
        default:
          assert(false);
        }
      }
      assert(starts_next_it == starts_next.end());
      assert(ends_next_it == ends_next.end());
#if !PYCENTAURUS
      assert(values_next_it == values_next.end());
#endif
#if !PYCENTAURUS
      delete &stack_tuple;
#endif
      release_bank();
    }
    assert(starts.empty());
    assert(ends.empty());
    assert(values.size() <= 1);
    assert(tags.size() <= 1);

    auto result = (values.empty()) ? 0 : std::move(values.front());
#if !PYCENTAURUS
    delete &stack_tuple_base;
#endif
    return result;
  }
  void *acquire_bank()
  {
    WindowBankEntry *banks = reinterpret_cast<WindowBankEntry *>(m_sub_window);
    while (true) {
      for (int i = 0; i < m_bank_num; i++) {
        if (banks[i].state == WindowBankState::Stage2_Unlocked) {
          if (banks[i].number == m_counter) {
            banks[i].state = WindowBankState::Stage3_Locked;
            invoke_transfer_listener(i, banks[i].number);
            //std::cout << "Bank " << banks[i].number << " reached Stage3" << std::endl;
            m_current_bank = i;
            m_counter++;
            return (char *)m_main_window + m_bank_size * i;
          }
        } else if (banks[i].state == WindowBankState::YouAreDone) {
          return NULL;
        }
      }
    }
  }
  void release_bank()
  {
    if (m_current_bank != -1) {
        WindowBankEntry *banks = reinterpret_cast<WindowBankEntry *>(m_sub_window);
        banks[m_current_bank].state.store(WindowBankState::Free);
        m_current_bank = -1;
    }
  }

public:
  Stage3Runner(const char *filename, size_t bank_size, int bank_num, int master_pid, void *context = nullptr, std::atomic<int> *counter = nullptr)
    : NonRecursiveReductionRunner(filename, bank_size, bank_num, master_pid, context, counter)
  {
    acquire_memory(false);
  }
  virtual ~Stage3Runner()
  {
    release_memory(false);
  }
  virtual void start() override
  {
    _start<Stage3Runner>();
  }
};
}
