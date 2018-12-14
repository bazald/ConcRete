#ifndef ZENI_CONCURRENCY_INTRUSIVE_STACK_HPP
#define ZENI_CONCURRENCY_INTRUSIVE_STACK_HPP

#include "../../Internal/Linkage.hpp"
#include "../../DWCAS.hpp"

namespace Zeni::Concurrency {

  template <typename TYPE, typename DELETER = std::default_delete<TYPE>>
  class Intrusive_Stack {
    Intrusive_Stack(const Intrusive_Stack &) = delete;
    Intrusive_Stack & operator=(const Intrusive_Stack &) = delete;

  public:
    typedef TYPE Type;
    typedef DELETER Deleter;

    struct Node {
      mutable const Type * next = nullptr;
    };

    Intrusive_Stack() noexcept = default;

    ~Intrusive_Stack() noexcept {
      typename DWCAS_Ptr<Type>::Ptr ptr;
      m_ptr.load(ptr);
      const Deleter deleter;
      while (ptr.ptr) {
        Type * const next = const_cast<Type *>(ptr->next);
        deleter(ptr.ptr);
        ptr.ptr = next;
      }
    }

    bool empty() const {
      typename DWCAS_Ptr<Type>::Ptr ptr;
      m_ptr.load(ptr);
      return ptr.ptr;
    }

    //int64_t size() const {
    //  return m_size.load(std::memory_order_relaxed);
    //}

    void push(TYPE * const ptr) {
      //m_size.fetch_add(1, std::memory_order_relaxed);
      typename DWCAS_Ptr<Type>::Ptr expected;
      m_ptr.load(expected);
      auto desired = expected.make_desired(ptr);
      desired->next = expected.ptr;
      while (!m_ptr.CAS(expected, desired)) {
        expected.update_desired(desired, ptr);
        desired->next = expected.ptr;
      }
    }

    TYPE * try_pop() {
      typename DWCAS_Ptr<Type>::Ptr expected;
      m_ptr.load(expected);
      typename DWCAS_Ptr<Type>::Ptr desired;
      do {
        if (!expected.ptr)
          return nullptr;
        desired = expected.make_desired(const_cast<Type *>(expected->next));
      } while (!m_ptr.CAS(expected, desired, DWCAS_Ptr<Type>::Memory_Order::Acquire));
      //m_size.fetch_sub(1, std::memory_order_relaxed);
      return expected.ptr;
    }

  private:
    DWCAS_Ptr<Type> m_ptr;
    //ZENI_CONCURRENCY_CACHE_ALIGN std::atomic_int64_t m_size = 0;
  };

}

#endif
