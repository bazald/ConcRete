#if defined(_MSC_VER) && defined(_M_AMD64)

#include <memory>

#include "jemalloc/jemalloc.h"

void * operator new(size_t size) noexcept(false) {
  return je_malloc(size);
}

void * operator new(size_t size, std::align_val_t alignment) noexcept(false) {
  return je_aligned_alloc(size_t(alignment), size);
}

void operator delete(void * ptr) noexcept {
  je_free(ptr);
}

void operator delete(void * ptr, std::align_val_t) noexcept {
  je_free(ptr);
}

#endif
