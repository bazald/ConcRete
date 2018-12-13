#include "Zeni/Concurrency/Memory_Pool.hpp"
#include "Zeni/Concurrency/Memory_Pools.hpp"

#if !defined(USE_MEMORY_POOLS) && defined(_WIN32)
#include "jemalloc/jemalloc.h"
#endif

void * operator new(size_t size) noexcept(false) {
#if defined(USE_MEMORY_POOLS)
  void * const ptr = Zeni::Concurrency::Memory_Pools::get_pool()->allocate(size);
  return ptr;
#elif defined(_WIN32)
  return je_malloc(size);
#else
  return malloc(size);
#endif
}

void * operator new(size_t size, std::align_val_t alignment) noexcept(false) {
#if defined(USE_MEMORY_POOLS)
  void * const ptr = Zeni::Concurrency::Memory_Pools::get_pool()->allocate(size, alignment);
  return ptr;
#elif defined(_WIN32)
  return je_aligned_alloc(size_t(alignment), size);
#else
  return std::aligned_alloc(size_t(alignment), size);
#endif
}

void operator delete(void * ptr) noexcept {
#if defined(USE_MEMORY_POOLS)
  if (ptr)
    return Zeni::Concurrency::Memory_Pools::get_pool()->release(ptr);
#elif defined(_WIN32)
  je_free(ptr);
#else
  free(ptr);
#endif
}

void operator delete(void * ptr, std::align_val_t alignment) noexcept {
#if defined(USE_MEMORY_POOLS)
  if (ptr)
    return Zeni::Concurrency::Memory_Pools::get_pool()->release(ptr, alignment);
#elif defined(_WIN32)
  je_free(ptr);
#else
  free(ptr);
#endif
}
