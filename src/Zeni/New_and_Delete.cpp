#include "Zeni/Concurrency/Memory_Pool.hpp"

void * operator new(size_t size) noexcept(false) {
  void * const ptr = Zeni::Concurrency::Memory_Pool::get().allocate(size);
  return ptr;
}

void * operator new(size_t size, std::align_val_t) noexcept(false) {
  void * const ptr = Zeni::Concurrency::Memory_Pool::get().allocate(size);
  return ptr;
}

void operator delete(void * ptr) noexcept {
  return Zeni::Concurrency::Memory_Pool::get().release(ptr);
}

void operator delete(void * ptr, std::align_val_t) noexcept {
  return Zeni::Concurrency::Memory_Pool::get().release(ptr);
}
