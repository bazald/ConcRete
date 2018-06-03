#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>  
#endif

#include "Zeni/Concurrency/Memory_Pool.hpp"

#include "Zeni/Utility.hpp"

#include <unordered_map>

namespace Zeni::Concurrency {

  template <class T> struct Mallocator {
    typedef T value_type;
    Mallocator() noexcept { } // default ctor not required
    template <class U> Mallocator(const Mallocator<U>&) noexcept { }
    template <class U> bool operator==(
      const Mallocator<U>&) const noexcept {
      return true;
    }
    template <class U> bool operator!=(
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

  class Memory_Pool_Pimpl {
    Memory_Pool_Pimpl(const Memory_Pool_Pimpl &rhs) = delete;
    Memory_Pool_Pimpl & operator=(const Memory_Pool_Pimpl &rhs) = delete;

  public:
    Memory_Pool_Pimpl() noexcept
    {
#ifdef _MSC_VER
      _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
      _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
      //_CrtSetBreakAlloc(167);
#endif
    }

    ~Memory_Pool_Pimpl() noexcept {
      clear();
    }

    /// free any cached memory blocks; return a count of the number of blocks freed
    size_t clear() noexcept {
      size_t count = 0;

      for (auto &freed : m_freed) {
        void * ptr = freed.second;
        while (ptr) {
          void * ptr2 = *reinterpret_cast<void **>(ptr);
          std::free(reinterpret_cast<size_t *>(ptr) - 1);
          ptr = ptr2;
          ++count;
        }
      }

      m_freed.clear();

      return count;
    }

    /// get a cached memory block or allocate one as needed
    void * allocate(size_t size) noexcept {
      if (size < sizeof(void *))
        size = sizeof(void *);

      auto &freed = m_freed[size];

      if (freed) {
        void * const ptr = freed;
        freed = *reinterpret_cast<void **>(ptr);
#ifndef NDEBUG
        fill(reinterpret_cast<size_t *>(ptr), 0xFA57F00D);
#endif
        return reinterpret_cast<size_t *>(ptr);
      }
      else {
        void * ptr = std::malloc(sizeof(size_t) + size);

        if (ptr) {
          *reinterpret_cast<size_t *>(ptr) = size;
#ifndef NDEBUG
          fill(reinterpret_cast<size_t *>(ptr) + 1, 0xED1B13BF);
#endif
          return reinterpret_cast<size_t *>(ptr) + 1;
        }

        return nullptr;
      }
    }

    /// return a memory block to be cached (and eventually freed)
    void release(void * const ptr) noexcept {
      auto &freed = m_freed[size_of(ptr)];

#ifndef NDEBUG
      fill(ptr, 0xDEADBEEF);
#endif
      *reinterpret_cast<void **>(ptr) = freed;
      freed = reinterpret_cast<size_t *>(ptr);
    }

    /// get the size of a memory block allocated with an instance of Pool
    size_t size_of(const void * const ptr) const noexcept {
      return reinterpret_cast<const size_t *>(ptr)[-1];
    }

    void fill(void * const dest, const uint32_t pattern) noexcept {
      unsigned char * dd = reinterpret_cast<unsigned char *>(dest);
      const unsigned char * const dend = dd + size_of(dest);

      while (dend - dd > 3) {
        *reinterpret_cast<uint32_t *>(dd) = pattern;
        dd += 4;
      }
      if (dd == dend)
        return;
      *dd = pattern >> 24;
      if (++dd == dend)
        return;
      *dd = (pattern >> 16) & 0xFF;
      if (++dd == dend)
        return;
      *dd = (pattern >> 8) & 0xFF;
    }

  private:
    std::unordered_map<size_t, void *, std::hash<size_t>, std::equal_to<size_t>, Mallocator<std::pair<const size_t, void *>>> m_freed;
  };

  const Memory_Pool_Pimpl * Memory_Pool::get_pimpl() const noexcept {
    return reinterpret_cast<const Memory_Pool_Pimpl *>(m_pimpl_storage);
  }

  Memory_Pool_Pimpl * Memory_Pool::get_pimpl() noexcept {
    return reinterpret_cast<Memory_Pool_Pimpl *>(m_pimpl_storage);
  }

  Memory_Pool::Memory_Pool() noexcept {
    new (&m_pimpl_storage) Memory_Pool_Pimpl;
  }

  Memory_Pool::~Memory_Pool() noexcept {
    static_assert(std::alignment_of<Memory_Pool_Pimpl>::value <= Memory_Pool::m_pimpl_align, "Memory_Pool::m_pimpl_align is too low.");
    ZENI_STATIC_WARNING(std::alignment_of<Memory_Pool_Pimpl>::value >= Memory_Pool::m_pimpl_align, "Memory_Pool::m_pimpl_align is too high.");

    static_assert(sizeof(Memory_Pool_Pimpl) <= sizeof(Memory_Pool::m_pimpl_storage), "Memory_Pool::m_pimpl_size too low.");
    ZENI_STATIC_WARNING(sizeof(Memory_Pool_Pimpl) >= sizeof(Memory_Pool::m_pimpl_storage), "Memory_Pool::m_pimpl_size too high.");

    get_pimpl()->~Memory_Pool_Pimpl();
  }

  size_t Memory_Pool::clear() noexcept {
    return get_pimpl()->clear();
  }

  void * Memory_Pool::allocate(const size_t size) noexcept {
    return get_pimpl()->allocate(size);
  }

  void Memory_Pool::release(void * const ptr) noexcept {
    return get_pimpl()->release(ptr);
  }

  size_t Memory_Pool::size_of(const void * const ptr) const noexcept {
    return get_pimpl()->size_of(ptr);
  }

}
