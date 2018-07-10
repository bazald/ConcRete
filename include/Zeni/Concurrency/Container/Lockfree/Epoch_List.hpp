#ifndef ZENI_CONCURRENCY_EPOCH_LIST_HPP
#define ZENI_CONCURRENCY_EPOCH_LIST_HPP

#include "Shared_Ptr.hpp"
#include "Unordered_List.hpp"
#include "Queue.hpp"

namespace Zeni::Concurrency {

  class Epoch_List {
    Epoch_List(const Epoch_List &) = delete;
    Epoch_List & operator=(const Epoch_List &) = delete;

  public:
    typedef Shared_Ptr<std::atomic_uint64_t> Token;

    static const uint64_t epoch_increment = 2;

    Epoch_List() noexcept = default;

    bool empty() const {
      return m_assigned_epochs.empty() && m_acquires.empty();
    }

    //int64_t size() const {
    //  return m_assigned_epochs.size();
    //}

    uint64_t front_and_acquire(const Token current_epoch) {
      //m_size.fetch_add(1, std::memory_order_relaxed);
      m_writers.fetch_add(1, std::memory_order_relaxed);

      uint64_t front = 0;
      m_assigned_epochs.front(front);

      m_acquires.push(current_epoch);
      continue_acquire(front, current_epoch);

      m_writers.fetch_sub(1, std::memory_order_relaxed);
      return front;
    }

    bool try_release(const Token epoch) {
      m_writers.fetch_add(1, std::memory_order_relaxed);

      uint64_t front = 0;
      m_assigned_epochs.front(front);

      continue_acquire(front, epoch);
      const bool success = m_assigned_epochs.try_erase(*Token::Lock(epoch));

      m_writers.fetch_sub(1, std::memory_order_relaxed);
      return success;
    }

  private:
    void continue_acquire(const uint64_t front, const Token epoch) {
      while (*Token::Lock(epoch) == 0) {
        Token current;
        if (!m_acquires.front(current))
          break;
        Token::Lock current_value(current);
        if (!current_value)
          break;
        uint64_t value = *current_value;
        uint64_t next_assignable = m_next_assignable.load(std::memory_order_relaxed);
        if (value == 0 && current_value->compare_exchange_strong(value, next_assignable, std::memory_order_relaxed, std::memory_order_relaxed))
          value = next_assignable;
        if (value == next_assignable)
          m_next_assignable.compare_exchange_strong(next_assignable, next_assignable + epoch_increment, std::memory_order_relaxed, std::memory_order_relaxed);
        m_assigned_epochs.push_front_if_gtewrt(front, *current_value);
        m_acquires.try_pop_if_equals(current);
      }
    }

    Unordered_List<uint64_t> m_assigned_epochs;
    std::atomic_uint64_t m_next_assignable = 1;
    Queue<Token> m_acquires;
    //std::atomic_int64_t m_size = 0;
    std::atomic_int64_t m_writers = 0;
  };
  
}

#endif
