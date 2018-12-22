#ifndef ZENI_CONCURRENCY_INTRUSIVE_SHARED_PTR_HPP
#define ZENI_CONCURRENCY_INTRUSIVE_SHARED_PTR_HPP

#include "../../Internal/Reclamation_Stack.hpp"

#include <iostream>
#include <thread>

namespace Zeni::Concurrency {

  template <typename TYPE>
  class Intrusive_Shared_Ptr;

  class ZENI_CONCURRENCY_CACHE_ALIGN Enable_Intrusive_Sharing : private Reclamation_Stack::Node {
    Enable_Intrusive_Sharing(const Enable_Intrusive_Sharing &) = delete;
    Enable_Intrusive_Sharing & operator=(const Enable_Intrusive_Sharing &) = delete;

  protected:
    Enable_Intrusive_Sharing() = default;

  public:
    int64_t decrement_refs() const {
      const uint64_t prev = m_refs.fetch_sub(1, std::memory_order_relaxed);
      if (prev > 1)
        return prev;
      assert(prev == 1);
      Reclamation_Stack::push(this);
      return prev;
    }

    int64_t try_increment_refs() const {
      uint64_t refs = m_refs.load(std::memory_order_relaxed);
      while (refs) {
        assert(refs != uint64_t(-1));
        if (m_refs.compare_exchange_weak(refs, refs + 1, std::memory_order_relaxed, std::memory_order_relaxed))
          break;
      }
      return refs;
    }

#ifndef NDEBUG
    bool check_refs() const {
      return m_refs.load(std::memory_order_relaxed) != 0;
    }
#endif

    mutable std::atomic_uint64_t m_refs = 1;
  };

  template <typename TYPE>
  class ZENI_CONCURRENCY_CACHE_ALIGN Intrusive_Shared_Ptr {
  public:
    class Lock {
      friend class Intrusive_Shared_Ptr<TYPE>;

    public:
      Lock() = default;

      ~Lock() {
        assert(std::this_thread::get_id() == m_thread);
        assert(!m_ptr || m_ptr->check_refs());
        if (m_ptr)
          m_ptr->decrement_refs();
      }

      Lock(const Lock &rhs)
        : m_ptr(rhs.m_ptr)
      {
        assert(rhs.m_thread == m_thread);
        assert(!rhs.m_ptr || rhs.m_ptr->check_refs());
        if (m_ptr)
          m_ptr->try_increment_refs();
      }

      Lock(Lock &&rhs)
        : m_ptr(rhs.m_ptr)
      {
        assert(rhs.m_thread == m_thread);
        assert(!rhs.m_ptr || rhs.m_ptr->check_refs());
        rhs.m_ptr = nullptr;
      }

      Lock & operator=(const Lock &rhs) {
        assert(std::this_thread::get_id() == m_thread);
        assert(rhs.m_thread == m_thread);
        assert(!m_ptr || m_ptr->check_refs());
        assert(!rhs.m_ptr || rhs.m_ptr->check_refs());
        Lock lock(rhs);
        std::swap(m_ptr, lock.m_ptr);
        return *this;
      }

      Lock & operator=(Lock &&rhs) {
        assert(std::this_thread::get_id() == m_thread);
        assert(rhs.m_thread == m_thread);
        assert(!m_ptr || m_ptr->check_refs());
        assert(!rhs.m_ptr || rhs.m_ptr->check_refs());
        std::swap(m_ptr, rhs.m_ptr);
        return *this;
      }

      explicit Lock(const Intrusive_Shared_Ptr<TYPE> &rhs) {
        do {
          m_ptr = rhs.m_ptr.load(std::memory_order_seq_cst);
        } while (m_ptr && !m_ptr->try_increment_refs());
      }

      explicit Lock(const Intrusive_Shared_Ptr<TYPE> &rhs, const std::memory_order order) {
        do {
          m_ptr = rhs.m_ptr.load(order);
        } while (m_ptr && !m_ptr->try_increment_refs());
      }

      Lock(TYPE * const ptr)
        : m_ptr(ptr)
      {
      }

      void swap(Lock &rhs) {
        assert(std::this_thread::get_id() == m_thread);
        assert(rhs.m_thread == m_thread);
        assert(!m_ptr || m_ptr->check_refs());
        assert(!rhs.m_ptr || rhs.m_ptr->check_refs());
        std::swap(m_ptr, rhs.m_ptr);
      }

      void reset() {
        assert(std::this_thread::get_id() == m_thread);
        assert(!m_ptr || m_ptr->check_refs());
        if (m_ptr)
          m_ptr->decrement_refs();
        m_ptr = nullptr;
      }

      TYPE * get() const {
        assert(std::this_thread::get_id() == m_thread);
        assert(!m_ptr || m_ptr->check_refs());
        return m_ptr;
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
        assert(!m_ptr || m_ptr->check_refs());
        return m_ptr;
      }

      bool operator==(const Lock &rhs) const {
        assert(std::this_thread::get_id() == m_thread);
        assert(rhs.m_thread == m_thread);
        assert(!m_ptr || m_ptr->check_refs());
        assert(!rhs.m_ptr || rhs.m_ptr->check_refs());
        if (!m_ptr)
          return !rhs;
        if (!rhs)
          return false;
        return get() == rhs.get();
      }

      bool operator!=(const Lock &rhs) const {
        assert(std::this_thread::get_id() == m_thread);
        assert(rhs.m_thread == m_thread);
        assert(!m_ptr || m_ptr->check_refs());
        assert(!rhs.m_ptr || rhs.m_ptr->check_refs());
        if (!m_ptr)
          return rhs.m_ptr;
        if (!rhs.m_ptr)
          return true;
        return get() != rhs.get();
      }

      bool operator<(const Lock &rhs) const {
        assert(std::this_thread::get_id() == m_thread);
        assert(rhs.m_thread == m_thread);
        assert(!m_ptr || m_ptr->check_refs());
        assert(!rhs.m_ptr || rhs.m_ptr->check_refs());
        if (!m_ptr)
          return rhs.m_ptr;
        if (!rhs.m_ptr)
          return false;
        return get() < rhs.get();
      }

      bool operator>(const Lock &rhs) const {
        assert(std::this_thread::get_id() == m_thread);
        assert(rhs.m_thread == m_thread);
        assert(!m_ptr || m_ptr->check_refs());
        assert(!rhs.m_ptr || rhs.m_ptr->check_refs());
        if (!m_ptr)
          return false;
        if (!rhs.m_ptr)
          return true;
        return get() > rhs.get();
      }

      bool operator<=(const Lock &rhs) const {
        assert(std::this_thread::get_id() == m_thread);
        assert(rhs.m_thread == m_thread);
        assert(!m_ptr || m_ptr->check_refs());
        assert(!rhs.m_ptr || rhs.m_ptr->check_refs());
        if (!m_ptr)
          return true;
        if (!rhs.m_ptr)
          return false;
        return get() <= rhs.get();
      }

      bool operator>=(const Lock &rhs) const {
        assert(std::this_thread::get_id() == m_thread);
        assert(rhs.m_thread == m_thread);
        assert(!m_ptr || m_ptr->check_refs());
        assert(!rhs.m_ptr || rhs.m_ptr->check_refs());
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
      TYPE * m_ptr = nullptr;
    };

    Intrusive_Shared_Ptr() = default;

    ~Intrusive_Shared_Ptr() {
      TYPE * const ptr = m_ptr.load(std::memory_order_relaxed);
      assert(!ptr || ptr->check_refs());
      if (ptr)
        ptr->decrement_refs();
    }

    Intrusive_Shared_Ptr(TYPE * const ptr)
      : m_ptr(ptr)
    {
    }

    Intrusive_Shared_Ptr(const typename Intrusive_Shared_Ptr<TYPE>::Lock &rhs)
      : m_ptr(rhs.m_ptr && rhs.m_ptr->try_increment_refs() ? rhs.m_ptr : nullptr)
    {
    }

    Intrusive_Shared_Ptr(const Intrusive_Shared_Ptr<TYPE> &rhs) {
      TYPE * const rhs_ptr = rhs.m_ptr.load(std::memory_order_relaxed);
      if (rhs_ptr && rhs_ptr->try_increment_refs())
        m_ptr.store(rhs_ptr);
    }

    typename Intrusive_Shared_Ptr<TYPE>::Lock load() const {
      return Intrusive_Shared_Ptr<TYPE>::Lock(*this);
    }

    typename Intrusive_Shared_Ptr<TYPE>::Lock load(const std::memory_order order) const {
      return Intrusive_Shared_Ptr<TYPE>::Lock(*this, order);
    }

    void store(const typename Intrusive_Shared_Ptr<TYPE>::Lock &rhs) {
      store(rhs, std::memory_order_seq_cst);
    }

    void store(const typename Intrusive_Shared_Ptr<TYPE>::Lock &rhs, const std::memory_order order) {
      Intrusive_Shared_Ptr<TYPE>::Lock expected = load(std::memory_order_relaxed);
      compare_exchange_strong(expected, rhs, order, std::memory_order_relaxed);
    }

    Intrusive_Shared_Ptr & operator=(const typename Intrusive_Shared_Ptr<TYPE>::Lock &rhs) {
      store(rhs);
      return *this;
    }

    Intrusive_Shared_Ptr & operator=(const Intrusive_Shared_Ptr<TYPE> &rhs) {
      store(Lock(rhs, std::memory_order_seq_cst), std::memory_order_seq_cst);
      return *this;
    }

    bool compare_exchange_strong(typename Intrusive_Shared_Ptr<TYPE>::Lock &expected, const typename Intrusive_Shared_Ptr<TYPE>::Lock desired) {
      return compare_exchange_strong(expected, desired, std::memory_order_seq_cst, std::memory_order_seq_cst);
    }

    bool compare_exchange_strong(typename Intrusive_Shared_Ptr<TYPE>::Lock &expected, const typename Intrusive_Shared_Ptr<TYPE>::Lock desired, const std::memory_order succ, const std::memory_order fail) {
      assert(!expected.m_ptr || expected.m_ptr->check_refs());
      assert(!desired.m_ptr || desired.m_ptr->check_refs());
      TYPE * const old_expected = expected.m_ptr;
      if (desired.m_ptr)
        desired.m_ptr->try_increment_refs();
      bool success = m_ptr.compare_exchange_strong(expected.m_ptr, desired.m_ptr, succ, fail);
      if (!success) {
        if (expected.m_ptr && !expected.m_ptr->try_increment_refs()) {
          expected.m_ptr = nullptr;
          expected = load(fail);
        }
        if (desired.m_ptr)
          desired.m_ptr->decrement_refs();
      }
      if (old_expected)
        old_expected->decrement_refs();
      return success;
    }

    bool operator==(const Intrusive_Shared_Ptr<TYPE> &rhs) const {
      return Lock(*this) == Lock(rhs);
    }

    bool operator!=(const Intrusive_Shared_Ptr<TYPE> &rhs) const {
      return Lock(*this) != Lock(rhs);
    }

    bool operator<(const Intrusive_Shared_Ptr<TYPE> &rhs) const {
      return Lock(*this) < Lock(rhs);
    }

    bool operator>(const Intrusive_Shared_Ptr<TYPE> &rhs) const {
      return Lock(*this) > Lock(rhs);
    }

    bool operator<=(const Intrusive_Shared_Ptr<TYPE> &rhs) const {
      return Lock(*this) <= Lock(rhs);
    }

    bool operator>=(const Intrusive_Shared_Ptr<TYPE> &rhs) const {
      return Lock(*this) >= Lock(rhs);
    }

    void reset() {
      TYPE * old_ptr = m_ptr.load(std::memory_order_relaxed);
      assert(old_ptr->check_refs());
      if (m_ptr.compare_exchange_strong(old_ptr, nullptr, std::memory_order_relaxed, std::memory_order_relaxed)) {
        if (old_ptr)
          old_ptr->decrement_refs();
      }
    }

  private:
    ZENI_CONCURRENCY_CACHE_ALIGN std::atomic<TYPE *> m_ptr = nullptr;
  };

  template <typename TYPE1, typename TYPE2>
  const typename Intrusive_Shared_Ptr<TYPE1>::Lock & dynamic_pointer_cast(const typename Intrusive_Shared_Ptr<TYPE2>::Lock &rhs) {
    dynamic_cast<TYPE1 *>(rhs.m_ptr);
    return reinterpret_cast<const typename Intrusive_Shared_Ptr<TYPE1>::Lock &>(rhs);
  }

  template <typename TYPE1, typename TYPE2>
  typename Intrusive_Shared_Ptr<TYPE1>::Lock & dynamic_pointer_cast(typename Intrusive_Shared_Ptr<TYPE2>::Lock &rhs) {
    dynamic_cast<TYPE1 *>(rhs.m_ptr);
    return reinterpret_cast<const typename Intrusive_Shared_Ptr<TYPE1>::Lock &>(rhs);
  }

  template <typename TYPE1, typename TYPE2>
  const typename Intrusive_Shared_Ptr<TYPE1>::Lock & static_pointer_cast(const typename Intrusive_Shared_Ptr<TYPE2>::Lock &rhs) {
    static_cast<TYPE1 *>(rhs.m_ptr);
    return reinterpret_cast<const typename Intrusive_Shared_Ptr<TYPE1>::Lock &>(rhs);
  }

  template <typename TYPE1, typename TYPE2>
  typename Intrusive_Shared_Ptr<TYPE1>::Lock & static_pointer_cast(typename Intrusive_Shared_Ptr<TYPE2>::Lock &rhs) {
    static_cast<TYPE1 *>(rhs.m_ptr);
    return reinterpret_cast<const typename Intrusive_Shared_Ptr<TYPE1>::Lock &>(rhs);
  }

  template <typename TYPE1, typename TYPE2>
  const typename Intrusive_Shared_Ptr<TYPE1>::Lock & reinterpret_pointer_cast(const typename Intrusive_Shared_Ptr<TYPE2>::Lock &rhs) {
    return reinterpret_cast<const typename Intrusive_Shared_Ptr<TYPE1>::Lock &>(rhs);
  }

  template <typename TYPE1, typename TYPE2>
  typename Intrusive_Shared_Ptr<TYPE1>::Lock & reinterpret_pointer_cast(typename Intrusive_Shared_Ptr<TYPE2>::Lock &rhs) {
    return reinterpret_cast<const typename Intrusive_Shared_Ptr<TYPE1>::Lock &>(rhs);
  }

}

template <typename TYPE>
std::ostream & operator<<(std::ostream &os, const typename Zeni::Concurrency::Intrusive_Shared_Ptr<TYPE>::Lock &ptr) {
  return os << ptr.get();
}

#endif
