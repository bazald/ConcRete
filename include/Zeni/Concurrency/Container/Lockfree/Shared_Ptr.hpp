#ifndef ZENI_CONCURRENCY_SHARED_PTR_HPP
#define ZENI_CONCURRENCY_SHARED_PTR_HPP

#include "../../Internal/Reclamation_Stacks.hpp"

#include <iostream>
#include <thread>

namespace Zeni::Concurrency {

  template <typename TYPE>
  class ZENI_CONCURRENCY_CACHE_ALIGN Shared_Ptr {
    struct Node;

  public:
    class Lock {
      friend class Shared_Ptr<TYPE>;

    public:
      Lock() = default;

      ~Lock() {
        assert(std::this_thread::get_id() == m_thread);
        assert(!m_ptr || bool(*m_ptr));
        if (m_ptr) {
          std::atomic_thread_fence(std::memory_order_release);
          m_ptr->decrement_refs();
        }
      }

      Lock(const Lock &rhs)
        : m_ptr(rhs.m_ptr)
      {
        assert(rhs.m_thread == m_thread);
        assert(!rhs.m_ptr || bool(*rhs.m_ptr));
        if (m_ptr)
          m_ptr->increment_refs();
      }

      Lock(Lock &&rhs)
        : m_ptr(rhs.m_ptr)
      {
        assert(rhs.m_thread == m_thread);
        assert(!rhs.m_ptr || bool(*rhs.m_ptr));
        rhs.m_ptr = nullptr;
      }

      Lock & operator=(const Lock &rhs) {
        assert(std::this_thread::get_id() == m_thread);
        assert(rhs.m_thread == m_thread);
        assert(!m_ptr || bool(*m_ptr));
        assert(!rhs.m_ptr || bool(*rhs.m_ptr));
        Lock lock(rhs);
        std::swap(m_ptr, lock.m_ptr);
        return *this;
      }

      Lock & operator=(Lock &&rhs) {
        assert(std::this_thread::get_id() == m_thread);
        assert(rhs.m_thread == m_thread);
        assert(!m_ptr || bool(*m_ptr));
        assert(!rhs.m_ptr || bool(*rhs.m_ptr));
        std::swap(m_ptr, rhs.m_ptr);
        return *this;
      }

      explicit Lock(const Shared_Ptr<TYPE> &rhs) {
        Node * const rhs_ptr = rhs.m_ptr.load(std::memory_order_relaxed);
        m_ptr = rhs_ptr && rhs_ptr->increment_refs() ? rhs_ptr : nullptr;
      }

      Lock(TYPE * const ptr)
        : m_ptr(ptr ? new Node(ptr) : nullptr)
      {
      }

      void swap(Lock &rhs) {
        assert(std::this_thread::get_id() == m_thread);
        assert(rhs.m_thread == m_thread);
        assert(!m_ptr || bool(*m_ptr));
        assert(!rhs.m_ptr || bool(*rhs.m_ptr));
        std::swap(m_ptr, rhs.m_ptr);
      }

      void reset() {
        assert(std::this_thread::get_id() == m_thread);
        assert(!m_ptr || bool(*m_ptr));
        if (m_ptr)
          m_ptr->decrement_refs();
        m_ptr = nullptr;
      }

      TYPE * get() const {
        assert(std::this_thread::get_id() == m_thread);
        assert(!m_ptr || bool(*m_ptr));
        return m_ptr ? m_ptr->get() : nullptr;
      }

      TYPE & operator*() const {
        assert(std::this_thread::get_id() == m_thread);
        return *get();
      }

      TYPE * operator->() const {
        assert(std::this_thread::get_id() == m_thread);
        return get();
      }

      TYPE & operator[](std::ptrdiff_t idx) {
        assert(std::this_thread::get_id() == m_thread);
        return get()[idx];
      }

      explicit operator bool() const {
        assert(std::this_thread::get_id() == m_thread);
        assert(!m_ptr || bool(*m_ptr));
        return m_ptr;
      }

      bool operator==(const Lock &rhs) const {
        assert(std::this_thread::get_id() == m_thread);
        assert(rhs.m_thread == m_thread);
        assert(!m_ptr || bool(*m_ptr));
        assert(!rhs.m_ptr || bool(*rhs.m_ptr));
        if (!m_ptr)
          return !rhs;
        if (!rhs)
          return false;
        return get() == rhs.get();
      }

      bool operator!=(const Lock &rhs) const {
        assert(std::this_thread::get_id() == m_thread);
        assert(rhs.m_thread == m_thread);
        assert(!m_ptr || bool(*m_ptr));
        assert(!rhs.m_ptr || bool(*rhs.m_ptr));
        if (!m_ptr)
          return rhs.m_ptr;
        if (!rhs.m_ptr)
          return true;
        return get() != rhs.get();
      }

      bool operator<(const Lock &rhs) const {
        assert(std::this_thread::get_id() == m_thread);
        assert(rhs.m_thread == m_thread);
        assert(!m_ptr || bool(*m_ptr));
        assert(!rhs.m_ptr || bool(*rhs.m_ptr));
        if (!m_ptr)
          return rhs.m_ptr;
        if (!rhs.m_ptr)
          return false;
        return get() < rhs.get();
      }

      bool operator>(const Lock &rhs) const {
        assert(std::this_thread::get_id() == m_thread);
        assert(rhs.m_thread == m_thread);
        assert(!m_ptr || bool(*m_ptr));
        assert(!rhs.m_ptr || bool(*rhs.m_ptr));
        if (!m_ptr)
          return false;
        if (!rhs.m_ptr)
          return true;
        return get() > rhs.get();
      }

      bool operator<=(const Lock &rhs) const {
        assert(std::this_thread::get_id() == m_thread);
        assert(rhs.m_thread == m_thread);
        assert(!m_ptr || bool(*m_ptr));
        assert(!rhs.m_ptr || bool(*rhs.m_ptr));
        if (!m_ptr)
          return true;
        if (!rhs.m_ptr)
          return false;
        return get() <= rhs.get();
      }

      bool operator>=(const Lock &rhs) const {
        assert(std::this_thread::get_id() == m_thread);
        assert(rhs.m_thread == m_thread);
        assert(!m_ptr || bool(*m_ptr));
        assert(!rhs.m_ptr || bool(*rhs.m_ptr));
        if (!m_ptr)
          return !rhs.m_ptr;
        if (!rhs.m_ptr)
          return true;
        return get() >= rhs.get();
      }

    private:
#ifndef NDEBUG
      const std::thread::id m_thread = std::this_thread::get_id();
#endif
      Node * m_ptr = nullptr;
    };

    Shared_Ptr() = default;

    ~Shared_Ptr() {
      Node * const ptr = m_ptr.load(std::memory_order_relaxed);
      if (ptr && bool(*ptr))
        ptr->decrement_refs();
    }

    Shared_Ptr(TYPE * const ptr)
      : m_ptr(ptr ? new Node(ptr) : nullptr)
    {
    }

    Shared_Ptr(const typename Shared_Ptr<TYPE>::Lock &rhs)
      : m_ptr(rhs.m_ptr && rhs.m_ptr->increment_refs() ? rhs.m_ptr : nullptr)
    {
    }

    Shared_Ptr(const Shared_Ptr<TYPE> &rhs) {
      Node * const rhs_ptr = rhs.m_ptr.load(std::memory_order_relaxed);
      if (rhs_ptr && rhs_ptr->increment_refs())
        m_ptr.store(rhs_ptr);
    }

    typename Shared_Ptr<TYPE>::Lock load() const {
      return Shared_Ptr<TYPE>::Lock(*this);
    }

    void store(const typename Shared_Ptr<TYPE>::Lock &rhs) {
      Shared_Ptr<TYPE>::Lock expected = load();
      compare_exchange_strong(expected, rhs);
    }

    Shared_Ptr & operator=(const typename Shared_Ptr<TYPE>::Lock &rhs) {
      store(rhs);
      return *this;
    }

    Shared_Ptr & operator=(const Shared_Ptr<TYPE> &rhs) {
      store(Lock(rhs));
      return *this;
    }

    bool compare_exchange_strong(typename Shared_Ptr<TYPE>::Lock &expected, const typename Shared_Ptr<TYPE>::Lock desired) {
      assert(!expected.m_ptr || bool(*expected.m_ptr));
      assert(!desired.m_ptr || bool(*desired.m_ptr));
      Node * const old_expected = expected.m_ptr;
      if (desired.m_ptr)
        desired.m_ptr->increment_refs();
      bool success = m_ptr.compare_exchange_strong(expected.m_ptr, desired.m_ptr, std::memory_order_relaxed, std::memory_order_relaxed);
      if (!success) {
        if (expected.m_ptr && !expected.m_ptr->increment_refs())
          expected.m_ptr = nullptr;
        if (desired.m_ptr)
          desired.m_ptr->decrement_refs();
      }
      if (old_expected)
        old_expected->decrement_refs();
      return success;
    }

    bool operator==(const Shared_Ptr<TYPE> &rhs) const {
      return Lock(*this) == Lock(rhs);
    }

    bool operator!=(const Shared_Ptr<TYPE> &rhs) const {
      return Lock(*this) != Lock(rhs);
    }

    bool operator<(const Shared_Ptr<TYPE> &rhs) const {
      return Lock(*this) < Lock(rhs);
    }

    bool operator>(const Shared_Ptr<TYPE> &rhs) const {
      return Lock(*this) > Lock(rhs);
    }

    bool operator<=(const Shared_Ptr<TYPE> &rhs) const {
      return Lock(*this) <= Lock(rhs);
    }

    bool operator>=(const Shared_Ptr<TYPE> &rhs) const {
      return Lock(*this) >= Lock(rhs);
    }

    void reset() {
      Node * old_ptr = m_ptr.load(std::memory_order_relaxed);
      while (!m_ptr.compare_exchange_weak(old_ptr, nullptr, std::memory_order_relaxed, std::memory_order_relaxed));
      if (old_ptr && bool(*old_ptr))
        old_ptr->decrement_refs();
    }

  private:
    ZENI_CONCURRENCY_CACHE_ALIGN std::atomic<Node *> m_ptr = nullptr;

    struct ZENI_CONCURRENCY_CACHE_ALIGN_TOGETHER Node : Reclamation_Stack::Node {
      Node(const Node &) = delete;
      Node & operator=(const Node &) = delete;

    public:
      Node(TYPE * const ptr_)
        : m_ptr(ptr_)
      {
        assert(m_ptr);
        std::atomic_thread_fence(std::memory_order_release);
      }

      void decrement_refs() {
        const uint64_t prev = m_refs.fetch_sub(1, std::memory_order_relaxed);
        if (prev > 1)
          return;
        assert(prev == 1);
        std::atomic_thread_fence(std::memory_order_acquire);
        delete m_ptr;
        Reclamation_Stacks::push(this);
      }

      bool increment_refs() {
        uint64_t refs = m_refs.load(std::memory_order_relaxed);
        while (refs) {
          assert(refs != uint64_t(-1));
          if (m_refs.compare_exchange_weak(refs, refs + 1, std::memory_order_relaxed, std::memory_order_relaxed))
            return true;
        }
        return false;
      }

      explicit operator bool() const {
        return m_refs.load(std::memory_order_relaxed) != 0;
      }

      TYPE * const get() const {
        return m_ptr;
      }

    private:
      TYPE * const m_ptr;
      std::atomic_uint64_t m_refs = 1;
    };
  };

}

template <typename TYPE>
std::ostream & operator<<(std::ostream &os, const typename Zeni::Concurrency::Shared_Ptr<TYPE>::Lock &ptr) {
  return os << ptr.get();
}

#endif
