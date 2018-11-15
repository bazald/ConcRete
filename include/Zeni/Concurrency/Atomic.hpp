#ifndef ZENI_CONCURRENCY_ATOMIC_HPP
#define ZENI_CONCURRENCY_ATOMIC_HPP

#include "Internal/Linkage.hpp"
#include "Internal/Concurrency.hpp"

#include <cstdint>

#if ZENI_CONCURRENCY != ZENI_CONCURRENCY_NONE
#include <atomic>
#endif

namespace Zeni::Concurrency {

  template <typename TYPE, bool RELAXED = false>
  class Atomic {
    Atomic(const Atomic<TYPE> &rhs) = delete;
    Atomic & operator=(const Atomic<TYPE> &rhs) = delete;

#if ZENI_CONCURRENCY != ZENI_CONCURRENCY_NONE
    static const std::memory_order m_order_both = RELAXED ? std::memory_order_relaxed : std::memory_order_acq_rel;
    static const std::memory_order m_order_load = RELAXED ? std::memory_order_relaxed : std::memory_order_acquire;
    static const std::memory_order m_order_store = RELAXED ? std::memory_order_relaxed : std::memory_order_release;
#endif

  public:
    typedef TYPE Type;

    Atomic(const Type value = Type())
      : m_value(value)
    {
    }

    bool compare_exchange_strong(Type &expected, const Type desired) {
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

    bool compare_exchange_weak(Type &expected, const Type desired) {
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

    Type fetch_add(const Type amount = Type(1)) {
#if ZENI_CONCURRENCY == ZENI_CONCURRENCY_NONE
      const Type prev = m_value;
      m_value += amount;
      return prev;
#else
      return m_value.fetch_add(amount, m_order_both);
#endif
    }

    Type fetch_sub(const Type amount = Type(1)) {
#if ZENI_CONCURRENCY == ZENI_CONCURRENCY_NONE
      const Type prev = m_value;
      m_value -= amount;
      return prev;
#else
      return m_value.fetch_sub(amount, m_order_both);
#endif
    }

    Type load() const {
#if ZENI_CONCURRENCY == ZENI_CONCURRENCY_NONE
      return m_value;
#else
      return m_value.load(m_order_load);
#endif
    }

    void store(const Type value) {
#if ZENI_CONCURRENCY == ZENI_CONCURRENCY_NONE
      m_value = value;
#else
      return m_value.store(value, m_order_store);
#endif
    }

  private:
#if ZENI_CONCURRENCY == ZENI_CONCURRENCY_NONE
    Type m_value;
#else
    ZENI_CONCURRENCY_CACHE_ALIGN std::atomic<Type> m_value;
#endif
  };

}

#endif
