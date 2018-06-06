#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>  
#endif

#include "Zeni/Concurrency/Memory_Pool.hpp"

namespace Zeni::Concurrency {

  Memory_Pool::Memory_Pool() noexcept
  {
#ifdef _MSC_VER
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    //_CrtSetBreakAlloc(167);
#endif
  }

  Memory_Pool::~Memory_Pool() noexcept {
    clear();
  }

  /// free any cached memory blocks; return a count of the number of blocks freed
  size_t Memory_Pool::clear() noexcept {
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
  void * Memory_Pool::allocate(size_t size) noexcept {
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
  void Memory_Pool::release(void * const ptr) noexcept {
    auto &freed = m_freed[size_of(ptr)];

#ifndef NDEBUG
    fill(ptr, 0xDEADBEEF);
#endif
    *reinterpret_cast<void **>(ptr) = freed;
    freed = reinterpret_cast<size_t *>(ptr);
  }

  /// get the size of a memory block allocated with an instance of Pool
  size_t Memory_Pool::size_of(const void * const ptr) const noexcept {
    return reinterpret_cast<const size_t *>(ptr)[-1];
  }

  void Memory_Pool::fill(void * const dest, const uint32_t pattern) noexcept {
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

}
