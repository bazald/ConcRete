#ifndef ZENI_CONCURRENCY_CAS2_HPP
#define ZENI_CONCURRENCY_CAS2_HPP

#include "Internal/Concurrency.hpp"

#include <atomic>
#include <memory>

#ifdef _MSC_VER
#include <intrin.h>
#else
inline bool _InterlockedCompareExchange128(volatile int64_t * Destination, const int64_t ExchangeHigh, const int64_t ExchangeLow, int64_t * ComparandResult)
{
  bool matched;
  asm volatile("lock cmpxchg16b %1"
    : "=@ccz"(matched), "+m"(*Destination), "+a"(ComparandResult[0]), "+d"(ComparandResult[1])
    : "b"(ExchangeLow), "c"(ExchangeHigh)
    : "cc");
  return matched;
}
#endif

namespace Zeni::Concurrency {

  template <typename TYPE>
  class ZENI_CONCURRENCY_CACHE_ALIGN_TOGETHER CAS2_Ptr {
  public:
    typedef TYPE Type;

    struct Ptr {
      int64_t counter = 0;
      Type * ptr = nullptr;

      const Type * operator->() const { return ptr; }
      Type * operator->() { return ptr; }

      const Type & operator*() const { return *ptr; }
      Type & operator*() { return *ptr; }

      Ptr make_desired(Type * const ptr) const {
        return { counter + 1, ptr };
      }

      void update_desired(Ptr &desired, Type * const ptr) const {
        desired.counter = counter + 1;
      }
    };

    enum Memory_Order : uint8_t {Relaxed = 0x0, Acquire = 0x1, Release = 0x2, Acq_Rel = 0x3};

    CAS2_Ptr() = default;

    CAS2_Ptr(Type * const ptr) : m_ptr(ptr) {}

    bool CAS(Ptr &expected, const Ptr &desired, const Memory_Order
#ifndef _MSC_VER
      memory_order
#endif
      = Memory_Order::Acq_Rel)
    {
#ifndef _MSC_VER
      if (memory_order & Memory_Order::Release)
        std::atomic_thread_fence(std::memory_order_release);
#endif
      const bool rv = _InterlockedCompareExchange128(&m_counter, reinterpret_cast<int64_t>(desired.ptr), desired.counter, &expected.counter) != 0;
#ifndef _MSC_VER
      if (memory_order & Memory_Order::Acquire)
        std::atomic_thread_fence(std::memory_order_acquire);
#endif
      return rv;
    }

    void load(Ptr &expected, const Memory_Order
#ifndef _MSC_VER
      memory_order
#endif
      = Memory_Order::Acq_Rel)
    {
      _InterlockedCompareExchange128(&m_counter, reinterpret_cast<int64_t>(expected.ptr), expected.counter, &expected.counter);
#ifndef _MSC_VER
      if (memory_order & Memory_Order::Acquire)
        std::atomic_thread_fence(std::memory_order_acquire);
#endif
    }

  private:
    volatile int64_t m_counter = 0;
    volatile Type * m_ptr = nullptr;
  };

}

#endif
