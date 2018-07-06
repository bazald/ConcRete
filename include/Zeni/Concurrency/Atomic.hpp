#ifndef ZENI_CONCURRENCY_ATOMIC_HPP
#define ZENI_CONCURRENCY_ATOMIC_HPP

#include "Internal/Linkage.hpp"
#include "Internal/Concurrency.hpp"

#include <cstdint>

#if ZENI_CONCURRENCY != ZENI_CONCURRENCY_NONE
#include <atomic>
#endif

namespace Zeni::Concurrency {

  template <bool RELAXED = false>
  class Atomic_int64_t {
    Atomic_int64_t(const Atomic_int64_t &rhs) = delete;
    Atomic_int64_t & operator=(const Atomic_int64_t &rhs) = delete;

#if ZENI_CONCURRENCY != ZENI_CONCURRENCY_NONE
    static const std::memory_order m_order_both = RELAXED ? std::memory_order_relaxed : std::memory_order_acq_rel;
    static const std::memory_order m_order_load = RELAXED ? std::memory_order_relaxed : std::memory_order_acquire;
    static const std::memory_order m_order_store = RELAXED ? std::memory_order_relaxed : std::memory_order_release;
#endif

  public:
    Atomic_int64_t(const int64_t value = 0)
      : m_value(value)
    {
    }

    bool compare_exchange_strong(int64_t &expected, const int64_t desired) {
#if ZENI_CONCURRENCY == ZENI_CONCURRENCY_NONE
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
#if ZENI_CONCURRENCY == ZENI_CONCURRENCY_NONE
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
#if ZENI_CONCURRENCY == ZENI_CONCURRENCY_NONE
      const int64_t prev = m_value;
      m_value += amount;
      return prev;
#else
      return m_value.fetch_add(amount, m_order_both);
#endif
    }

    int64_t fetch_sub(const int64_t amount = 1) {
#if ZENI_CONCURRENCY == ZENI_CONCURRENCY_NONE
      const int64_t prev = m_value;
      m_value -= amount;
      return prev;
#else
      return m_value.fetch_sub(amount, m_order_both);
#endif
    }

    int64_t load() const {
#if ZENI_CONCURRENCY == ZENI_CONCURRENCY_NONE
      return m_value;
#else
      return m_value.load(m_order_load);
#endif
    }

    void store(const int64_t value) {
#if ZENI_CONCURRENCY == ZENI_CONCURRENCY_NONE
      m_value = value;
#else
      return m_value.store(value, m_order_store);
#endif
    }

  private:
#if ZENI_CONCURRENCY == ZENI_CONCURRENCY_NONE
    int64_t m_value;
#else
    std::atomic_int64_t m_value;
#endif
  };

}

#endif
