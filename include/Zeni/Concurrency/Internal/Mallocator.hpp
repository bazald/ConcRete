#ifndef ZENI_CONCURRENCY_MALLOCATOR_HPP
#define ZENI_CONCURRENCY_MALLOCATOR_HPP

#include <cstddef>
#include <cstdlib>
#include <new>

#ifdef _WIN32
#include "jemalloc/jemalloc.h"
#endif

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

    typedef T value_type;
    Mallocator() noexcept { } // default ctor not required
    template <typename U> Mallocator(const Mallocator<U>&) noexcept { }
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

    typedef T value_type;
    Mallocator_Aligned() noexcept { } // default ctor not required
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
      void * const pv = std::aligned_alloc(size_t(alignment), n * sizeof(T));
      if (!pv) { throw std::bad_alloc(); }
      return static_cast<T *>(pv);
    }
    void deallocate(T * const p, size_t) const noexcept {
      free(p);
    }
  };

#ifdef _WIN32
  template <typename T, std::align_val_t alignment> struct Jemallocator_Aligned;

  template <typename T> struct Jemallocator {
    template <std::align_val_t alignment>
    struct Aligned {
      typedef Jemallocator_Aligned<T, alignment> Allocator;
    };
    struct Unaligned {
      typedef Jemallocator<T> Allocator;
    };

    typedef T value_type;
    Jemallocator() noexcept { } // default ctor not required
    template <typename U> Jemallocator(const Jemallocator<U>&) noexcept { }
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

    typedef T value_type;
    Jemallocator_Aligned() noexcept { } // default ctor not required
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
#else
  template <typename T, std::align_val_t alignment> struct Jemallocator_Aligned;

  template <typename T> struct Jemallocator {
    template <std::align_val_t alignment>
    struct Aligned {
      typedef Jemallocator_Aligned<T, alignment> Allocator;
    };
    struct Unaligned {
      typedef Jemallocator<T> Allocator;
    };

    typedef T value_type;
    Jemallocator() noexcept { } // default ctor not required
    template <typename U> Jemallocator(const Jemallocator<U>&) noexcept { }
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
      void * const pv = malloc(n * sizeof(T));
      if (!pv) { throw std::bad_alloc(); }
      return static_cast<T *>(pv);
    }
    void deallocate(T * const p, size_t) const noexcept {
      free(p);
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

    typedef T value_type;
    Jemallocator_Aligned() noexcept { } // default ctor not required
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
      void * const pv = aligned_alloc(size_t(alignment), n * sizeof(T));
      if (!pv) { throw std::bad_alloc(); }
      return static_cast<T *>(pv);
    }
    void deallocate(T * const p, size_t) const noexcept {
      free(p);
    }
  };
#endif

  template <typename ALLOCATOR> struct Allocated {
    template <std::align_val_t alignment>
    struct Aligned {
      typedef typename ALLOCATOR::template Aligned<alignment>::Allocator Allocator;
    };

    struct Unaligned {
      typedef typename ALLOCATOR::Unaligned::Allocator Allocator;
    };

    static void* operator new(size_t size) {
      return ALLOCATOR().allocate(size);
    }

    static void* operator new[](size_t size) {
      return ALLOCATOR().allocate(size);
    }

    static void operator delete(void* ptr, size_t size) {
      ALLOCATOR().deallocate(reinterpret_cast<typename ALLOCATOR::value_type *>(ptr), size);
    }

    static void operator delete[](void* ptr, size_t size) {
      ALLOCATOR().deallocate(reinterpret_cast<typename ALLOCATOR::value_type *>(ptr), size);
    }
  };

}

#endif
