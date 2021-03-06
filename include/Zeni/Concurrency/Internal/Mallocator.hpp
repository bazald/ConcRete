#ifndef ZENI_CONCURRENCY_MALLOCATOR_HPP
#define ZENI_CONCURRENCY_MALLOCATOR_HPP

#include <cstddef>
#include <cstdlib>
#include <new>

#define JEMALLOC_NO_DEMANGLE
#include "jemalloc/jemalloc.h"

namespace Zeni::Concurrency {

  template <typename T, std::align_val_t alignment> struct Mallocator_Aligned;

  template <typename T> struct Mallocator {
    template <std::align_val_t alignment>
    struct Aligned {
      typedef Mallocator_Aligned<T, alignment> Allocator;
    };
    struct Unaligned {
      typedef Mallocator<T> Allocator;
    };

    template<typename U>
    struct rebind { typedef Mallocator<U> other; };

    typedef T value_type;
    Mallocator() noexcept { } // default ctor not required
    template <typename U> Mallocator(const Mallocator<U>&) noexcept { }
    template <typename U, std::align_val_t A> Mallocator(const Mallocator_Aligned<U, A>&) noexcept { }
    template <typename U> bool operator==(
      const Mallocator<U>&) const noexcept {
      return true;
    }
    template <typename U> bool operator!=(
      const Mallocator<U>&) const noexcept {
      return false;
    }

    T * allocate(const size_t n) const {
      if (n == 0) { return nullptr; }
      if (n > static_cast<size_t>(-1) / sizeof(T)) {
        throw std::bad_array_new_length();
      }
      void * const pv = malloc(n * sizeof(T));
      if (!pv) { throw std::bad_alloc(); }
      return static_cast<T *>(pv);
    }
    void deallocate(T * const p, size_t) const noexcept {
      free(p);
    }
  };

  template <typename T, std::align_val_t alignment> struct Mallocator_Aligned {
    template <std::align_val_t alignment2>
    struct Aligned {
      typedef Mallocator_Aligned<T, alignment2> Allocator;
    };
    struct Unaligned {
      typedef Mallocator<T> Allocator;
    };

    template<typename U>
    struct rebind { typedef Mallocator<U> other; };

    typedef T value_type;
    Mallocator_Aligned() noexcept { } // default ctor not required
    template <typename U> Mallocator_Aligned(const Mallocator<U>&) noexcept { }
    template <typename U, std::align_val_t A> Mallocator_Aligned(const Mallocator_Aligned<U, A>&) noexcept { }
    template <typename U, std::align_val_t A> bool operator==(
      const Mallocator_Aligned<U, A>&) const noexcept {
      return true;
    }
    template <typename U, std::align_val_t A> bool operator!=(
      const Mallocator_Aligned<U, A>&) const noexcept {
      return false;
    }

    T * allocate(const size_t n) const {
      if (n == 0) { return nullptr; }
      if (n > static_cast<size_t>(-1) / sizeof(T)) {
        throw std::bad_array_new_length();
      }
      void * const pv =
#if defined(_MSC_VER) && !defined(NDEBUG)
        _aligned_malloc_dbg(n * sizeof(T), size_t(alignment), __FILE__, __LINE__);
#elif defined(_MSC_VER)
        _aligned_malloc(n * sizeof(T), size_t(alignment));
#else
        std::aligned_alloc(size_t(alignment), n * sizeof(T));
#endif
      if (!pv) { throw std::bad_alloc(); }
      return static_cast<T *>(pv);
    }
    void deallocate(T * const p, size_t) const noexcept {
#if defined(_MSC_VER) && !defined(NDEBUG)
      _aligned_free_dbg(p);
#elif defined(_MSC_VER)
      _aligned_free(p);
#else
      free(p);
#endif
    }
  };

  template <typename T, std::align_val_t alignment> struct Jemallocator_Aligned;

  template <typename T> struct Jemallocator {
    template <std::align_val_t alignment>
    struct Aligned {
      typedef Jemallocator_Aligned<T, alignment> Allocator;
    };
    struct Unaligned {
      typedef Jemallocator<T> Allocator;
    };

    template<typename U>
    struct rebind { typedef Jemallocator<U> other; };

    typedef T value_type;
    Jemallocator() noexcept { } // default ctor not required
    template <typename U> Jemallocator(const Jemallocator<U>&) noexcept { }
    template <typename U, std::align_val_t A> Jemallocator(const Jemallocator_Aligned<U, A>&) noexcept { }
    template <typename U> bool operator==(
      const Jemallocator<U>&) const noexcept {
      return true;
    }
    template <typename U> bool operator!=(
      const Jemallocator<U>&) const noexcept {
      return false;
    }

    T * allocate(const size_t n) const {
      if (n == 0) { return nullptr; }
      if (n > static_cast<size_t>(-1) / sizeof(T)) {
        throw std::bad_array_new_length();
      }
      void * const pv = je_malloc(n * sizeof(T));
      if (!pv) { throw std::bad_alloc(); }
      return static_cast<T *>(pv);
    }
    void deallocate(T * const p, size_t) const noexcept {
      je_free(p);
    }
  };

  template <typename T, std::align_val_t alignment> struct Jemallocator_Aligned {
    template <std::align_val_t alignment2>
    struct Aligned {
      typedef Jemallocator_Aligned<T, alignment2> Allocator;
    };
    struct Unaligned {
      typedef Jemallocator<T> Allocator;
    };

    template<typename U>
    struct rebind { typedef Jemallocator<U> other; };

    typedef T value_type;
    Jemallocator_Aligned() noexcept { } // default ctor not required
    template <typename U> Jemallocator_Aligned(const Jemallocator<U>&) noexcept { }
    template <typename U, std::align_val_t A> Jemallocator_Aligned(const Jemallocator_Aligned<U, A>&) noexcept { }
    template <typename U, std::align_val_t A> bool operator==(
      const Jemallocator_Aligned<U, A>&) const noexcept {
      return true;
    }
    template <typename U, std::align_val_t A> bool operator!=(
      const Jemallocator_Aligned<U, A>&) const noexcept {
      return false;
    }

    T * allocate(const size_t n) const {
      if (n == 0) { return nullptr; }
      if (n > static_cast<size_t>(-1) / sizeof(T)) {
        throw std::bad_array_new_length();
      }
      void * const pv = je_aligned_alloc(size_t(alignment), n * sizeof(T));
      if (!pv) { throw std::bad_alloc(); }
      return static_cast<T *>(pv);
    }
    void deallocate(T * const p, size_t) const noexcept {
      je_free(p);
    }
  };

  template <typename ALLOCATOR> struct Allocated {
    typedef typename ALLOCATOR::template rebind<char>::other Allocator;

    template <std::align_val_t alignment>
    struct Aligned {
      typedef typename ALLOCATOR::template Aligned<alignment>::Allocator Allocator;
    };

    struct Unaligned {
      typedef typename ALLOCATOR::Unaligned::Allocator Allocator;
    };

    static void* operator new(size_t size) {
      return Allocator().allocate(size);
    }

    static void* operator new[](size_t size) {
      return Allocator().allocate(size);
    }

    static void operator delete(void* ptr, size_t size) {
      Allocator().deallocate(reinterpret_cast<char *>(ptr), size);
    }

    static void operator delete[](void* ptr, size_t size) {
      Allocator().deallocate(reinterpret_cast<char *>(ptr), size);
    }
  };

}

#endif
