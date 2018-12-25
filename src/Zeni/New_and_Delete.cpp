//#define DISABLE_JEMALLOC

#if defined(DISABLE_JEMALLOC)
#if defined(_MSC_VER)
#include <crtdbg.h>
#endif
#include <cstddef>
#include <cstdlib>
#else
#define JEMALLOC_NO_DEMANGLE
#include "jemalloc/jemalloc.h"
#endif

void * operator new(size_t size) noexcept(false) {
#if defined(DISABLE_JEMALLOC)
  return malloc(size);
#else
  return je_malloc(size);
#endif
}

void * operator new(size_t size, std::align_val_t alignment) noexcept(false) {
#if defined(DISABLE_JEMALLOC) && defined(_MSC_VER)
  return _aligned_malloc_dbg(size, size_t(alignment), __FILE__, __LINE__);
#elif defined(DISABLE_JEMALLOC)
  return std::aligned_alloc(size_t(alignment), size);
#else
  return je_aligned_alloc(size_t(alignment), size);
#endif
}

void operator delete(void * ptr, size_t size) noexcept {
#if defined(DISABLE_JEMALLOC)
  free(ptr);
#else
  je_free(ptr);
#endif
}

void operator delete(void * ptr, size_t size, std::align_val_t alignment) noexcept {
#if defined(DISABLE_JEMALLOC) && defined(_MSC_VER)
  _aligned_free_dbg(ptr);
#elif defined(DISABLE_JEMALLOC)
  free(ptr);
#else
  je_free(ptr);
#endif
}
