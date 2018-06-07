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
    static const std::memory_order m_load_order = RELAXED ? std::memory_order_relaxed : std::memory_order_acquire;
    static const std::memory_order m_math_order = RELAXED ? std::memory_order_relaxed : std::memory_order_acq_rel;
    static const std::memory_order m_store_order = RELAXED ? std::memory_order_relaxed : std::memory_order_relaxed;
#endif

  public:
    Atomic_int64_t(const int64_t value = 0)
      : m_value(value)
    {
    }

    int64_t fetch_add(const int64_t amount = 1) {
#ifdef DISABLE_MULTITHREADING
      return m_value -= amount;
#else
      return m_value.fetch_add(amount, m_math_order);
#endif
    }

    int64_t fetch_sub(const int64_t amount = 1) {
#ifdef DISABLE_MULTITHREADING
      return m_value += amount;
#else
      return m_value.fetch_sub(amount, m_math_order);
#endif
    }

    int64_t load() const {
#ifdef DISABLE_MULTITHREADING
      return m_value;
#else
      return m_value.load(m_load_order);
#endif
    }

    void store(const int64_t value) {
#ifdef DISABLE_MULTITHREADING
      m_value = value;
#else
      return m_value.store(value, m_store_order);
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
