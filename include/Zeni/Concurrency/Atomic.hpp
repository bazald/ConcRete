#ifndef ZENI_CONCURRENCY_ATOMIC_HPP
#define ZENI_CONCURRENCY_ATOMIC_HPP

#include "Internal/Linkage.hpp"

#include <cstdint>

#ifndef DISABLE_MULTITHREADING
#include <atomic>
#endif

namespace Zeni::Concurrency {

  template <bool RELAXED = false>
  class Atomic_int64_t {
    Atomic_int64_t(const Atomic_int64_t &rhs) = delete;
    Atomic_int64_t & operator=(const Atomic_int64_t &rhs) = delete;

#ifndef DISABLE_MULTITHREADING
    static const std::memory_order m_order_both = RELAXED ? std::memory_order_relaxed : std::memory_order_acq_rel;
    static const std::memory_order m_order_load = RELAXED ? std::memory_order_relaxed : std::memory_order_acquire;
    static const std::memory_order m_order_store = RELAXED ? std::memory_order_relaxed : std::memory_order_relaxed;
#endif

  public:
    Atomic_int64_t(const int64_t value = 0)
      : m_value(value)
    {
    }

    bool compare_exchange_strong(int64_t &expected, const int64_t desired) {
#ifdef DISABLE_MULTITHREADING
      if (m_value == expected) {
        m_value = desired;
        return true;
      }
      else {
        expected = m_value;
        return false;
      }
#else
      return m_value.compare_exchange_strong(expected, desired, m_order_both);
#endif
    }

    bool compare_exchange_weak(int64_t &expected, const int64_t desired) {
#ifdef DISABLE_MULTITHREADING
      if (m_value == expected) {
        m_value = desired;
        return true;
      }
      else {
        expected = m_value;
        return false;
      }
#else
      return m_value.compare_exchange_weak(expected, desired, m_order_both);
#endif
    }

    int64_t fetch_add(const int64_t amount = 1) {
#ifdef DISABLE_MULTITHREADING
      const int64_t prev = m_value;
      m_value += amount;
      return prev;
#else
      return m_value.fetch_add(amount, m_order_both);
#endif
    }

    int64_t fetch_sub(const int64_t amount = 1) {
#ifdef DISABLE_MULTITHREADING
      const int64_t prev = m_value;
      m_value -= amount;
      return prev;
#else
      return m_value.fetch_sub(amount, m_order_both);
#endif
    }

    int64_t load() const {
#ifdef DISABLE_MULTITHREADING
      return m_value;
#else
      return m_value.load(m_order_load);
#endif
    }

    void store(const int64_t value) {
#ifdef DISABLE_MULTITHREADING
      m_value = value;
#else
      return m_value.store(value, m_order_store);
#endif
    }

  private:
#ifdef DISABLE_MULTITHREADING
    int64_t m_value;
#else
    std::atomic_int64_t m_value;
#endif
  };

}

#endif
