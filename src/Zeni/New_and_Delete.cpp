#include "Zeni/Concurrency/Memory_Pool.hpp"

#if defined(DISABLE_MEMORY_POOLS)
#define JEMALLOC_NO_DEMANGLE
#include "jemalloc/jemalloc.h"
#endif

void * operator new(size_t size) noexcept(false) {
#if defined(DISABLE_MEMORY_POOLS)
  return je_malloc(size);
#else
  void * const ptr = Zeni::Concurrency::Memory_Pool::allocate(size);
  return ptr;
#endif
}

void * operator new(size_t size, std::align_val_t alignment) noexcept(false) {
#if defined(DISABLE_MEMORY_POOLS)
  return je_aligned_alloc(size_t(alignment), size);
#else
  void * const ptr = Zeni::Concurrency::Memory_Pool::allocate(size, alignment);
  return ptr;
#endif
}

void operator delete(void * ptr, size_t size) noexcept {
#if defined(DISABLE_MEMORY_POOLS)
  je_free(ptr);
#else
  if (ptr)
    return Zeni::Concurrency::Memory_Pool::release(ptr, size);
#endif
}

void operator delete(void * ptr, size_t size, std::align_val_t alignment) noexcept {
#if defined(DISABLE_MEMORY_POOLS)
  je_free(ptr);
#else
  if (ptr)
    return Zeni::Concurrency::Memory_Pool::release(ptr, size, alignment);
#endif
}
