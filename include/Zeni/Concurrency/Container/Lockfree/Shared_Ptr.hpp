#ifndef ZENI_CONCURRENCY_SHARED_PTR_HPP
#define ZENI_CONCURRENCY_SHARED_PTR_HPP

#include "../../Internal/Reclamation_Stacks.hpp"

#include <iostream>

namespace Zeni::Concurrency {

  template <typename TYPE>
  class Shared_Ptr {
    struct Node;

  public:
    class Lock {
      Lock(const Lock &) = delete;
      Lock & operator=(const Lock &) = delete;

    public:
      Lock() = default;

      ~Lock() {
        m_writers.fetch_add(1, std::memory_order_relaxed);
        if (m_ptr)
          m_ptr->decrement_refs();
        m_writers.fetch_sub(1, std::memory_order_relaxed);
      }

      Lock(Lock &&rhs)
        : m_ptr(rhs.m_ptr)
      {
        rhs.m_ptr = nullptr;
      }

      Lock & operator=(Lock &&rhs) {
        std::swap(m_ptr, rhs.m_ptr);
      }

      Lock(const Shared_Ptr<TYPE> &rhs) {
        m_writers.fetch_add(1, std::memory_order_relaxed);
        m_ptr = rhs.m_ptr && rhs.m_ptr->increment_refs() ? rhs.m_ptr : nullptr;
        m_writers.fetch_sub(1, std::memory_order_relaxed);
      }

      Lock & operator=(const Shared_Ptr<TYPE> &rhs) {
        Lock lock(rhs);
        std::swap(m_ptr, lock.m_ptr);
      }

      void swap(Lock &rhs) {
        std::swap(m_ptr, rhs.m_ptr);
      }

      void reset() {
        if (m_ptr)
          m_ptr->decrement_refs();
        m_ptr = nullptr;
      }

      Node * get() const {
        return m_ptr;
      }

      TYPE & operator*() const { return *get(); }
      TYPE * operator->() const { return get(); }
      TYPE & operator[](std::ptrdiff_t idx) { return get()[idx]; }

      explicit operator bool() const {
        return m_ptr;
      }

      bool operator==(const Lock &rhs) const {
        if (!m_ptr)
          return !rhs;
        if (!rhs)
          return false;
        return *m_ptr == *rhs;
      }

      bool operator!=(const Lock &rhs) const {
        if (!m_ptr)
          return rhs.m_ptr;
        if (!rhs.m_ptr)
          return true;
        return *m_ptr != *rhs.m_ptr;
      }

      bool operator<(const Lock &rhs) const {
        if (!m_ptr)
          return rhs.m_ptr;
        if (!rhs.m_ptr)
          return false;
        return *m_ptr < *rhs.m_ptr;
      }

      bool operator>(const Lock &rhs) const {
        if (!m_ptr)
          return false;
        if (!rhs.m_ptr)
          return true;
        return *m_ptr > *rhs.m_ptr;
      }

      bool operator<=(const Lock &rhs) const {
        if (!m_ptr)
          return true;
        if (!rhs.m_ptr)
          return false;
        return *m_ptr <= *rhs.m_ptr;
      }

      bool operator>=(const Lock &rhs) const {
        if (!m_ptr)
          return !rhs.m_ptr;
        if (!rhs.m_ptr)
          return true;
        return *m_ptr >= *rhs.m_ptr;
      }

    private:
      Node * m_ptr = nullptr;
    };

    Shared_Ptr() = default;

    Shared_Ptr(TYPE * const ptr)
      : m_ptr(new Node(ptr))
    {
    }

    Shared_Ptr(const typename Shared_Ptr<TYPE>::Lock &rhs) {
      m_writers.fetch_add(1, std::memory_order_relaxed);
      m_ptr.store(rhs.m_ptr && rhs.m_ptr->increment_refs() ? rhs.m_ptr : nullptr, std::memory_order_relaxed);
      m_writers.fetch_sub(1, std::memory_order_relaxed);
    }

    Shared_Ptr(const Shared_Ptr<TYPE> &rhs) {
      m_writers.fetch_add(1, std::memory_order_relaxed);
      Node * const rhs_ptr = rhs.m_ptr.load(std::memory_order_relaxed);
      m_ptr.store(rhs_ptr && rhs_ptr->increment_refs() ? rhs_ptr : nullptr, std::memory_order_relaxed);
      m_writers.fetch_sub(1, std::memory_order_relaxed);
    }

    Shared_Ptr & operator=(const typename Shared_Ptr<TYPE>::Lock &rhs) {
      m_writers.fetch_add(1, std::memory_order_relaxed);
      Node * old_ptr = m_ptr.load(std::memory_order_relaxed);
      while (!m_ptr.compare_exchange_weak(old_ptr, rhs.m_ptr, std::memory_order_relaxed, std::memory_order_relaxed));
      if (old_ptr)
        old_ptr->decrement_refs();
      m_writers.fetch_sub(1, std::memory_order_relaxed);
    }

    Shared_Ptr & operator=(const Shared_Ptr<TYPE> &rhs) {
      m_writers.fetch_add(1, std::memory_order_relaxed);
      Node * rhs_ptr = rhs.m_ptr.load(std::memory_order_relaxed);
      if (rhs_ptr && !rhs_ptr->increment_refs())
        rhs_ptr = nullptr;
      Node * old_ptr = m_ptr.load(std::memory_order_relaxed);
      while (!m_ptr.compare_exchange_weak(old_ptr, rhs_ptr, std::memory_order_relaxed, std::memory_order_relaxed));
      if (old_ptr)
        old_ptr->decrement_refs();
      m_writers.fetch_sub(1, std::memory_order_relaxed);
    }

    bool compare_exchange_strong(typename Shared_Ptr<TYPE>::Lock &expected, const Shared_Ptr<TYPE> &desired) {
      m_writers.fetch_add(1, std::memory_order_relaxed);
      Node * desired_ptr = desired.m_ptr.load(std::memory_order_relaxed);
      if (desired_ptr && !desired_ptr->increment_refs())
        desired_ptr = nullptr;
      Node * expected_ptr_copy = expected.get();
      bool success = m_ptr.compare_exchange_strong(expected_ptr_copy, desired_ptr, std::memory_order_relaxed, std::memory_order_relaxed);
      if (success) {
        if (expected_ptr_copy)
          expected_ptr_copy->decrement_refs();
      }
      else {
        Shared_Ptr<TYPE>::Lock replacement(expected_ptr_copy);
        expected.swap(replacement);
        if (desired_ptr)
          desired_ptr->decrement_refs();
      }
      m_writers.fetch_sub(1, std::memory_order_relaxed);
      return success;
    }

    void reset() {
      m_writers.fetch_add(1, std::memory_order_relaxed);
      Node * old_ptr = m_ptr.load(std::memory_order_relaxed);
      while (!m_ptr.compare_exchange_weak(old_ptr, nullptr, std::memory_order_relaxed, std::memory_order_relaxed));
      if (old_ptr)
        old_ptr->decrement_refs();
      m_writers.fetch_sub(1, std::memory_order_relaxed);
    }

  private:
    std::atomic<Node *> m_ptr = nullptr;
    inline static std::atomic_uint64_t m_writers = 0;

    struct Node : Reclamation_Stack::Node {
      Node(TYPE * const ptr_)
        : m_ptr(ptr_)
      {
      }

      ~Node() {
        delete m_ptr.load(std::memory_order_acquire);
      }

      void decrement_refs() {
        if (m_refs.fetch_sub(1, std::memory_order_relaxed) != 1)
          return;
        if (m_writers.load(std::memory_order_relaxed) == 1)
          delete this;
        else
          Reclamation_Stacks::push(this);
      }

      bool increment_refs() {
        uint64_t refs = m_refs.load(std::memory_order_relaxed);
        while (refs) {
          if (m_refs.compare_exchange_weak(refs, refs + 1, std::memory_order_relaxed))
            return true;
        }
        return false;
      }

      explicit operator bool() const {
        return m_refs.load(std::memory_order_relaxed) != 0;
      }

    private:
      std::atomic<TYPE *> m_ptr = nullptr;
      std::atomic_uint64_t m_refs = 1;
    };
  };

}

template <typename TYPE>
std::ostream & operator<<(std::ostream &os, const typename Zeni::Concurrency::Shared_Ptr<TYPE>::Lock &ptr) {
  return os << ptr.get();
}

#endif
