#include "Zeni/Concurrency/Internal/Memory_Pool_Impl.hpp"

namespace Zeni::Concurrency {

  void Memory_Pool::clear() noexcept {
    Memory_Pool_Impl::get().clear();
  }

  void Memory_Pool::rotate() noexcept {
    Memory_Pool_Impl::get().rotate();
  }

  void * Memory_Pool::allocate(size_t size) noexcept {
    return Memory_Pool_Impl::get().allocate(size);
  }

  void * Memory_Pool::allocate(size_t size, const std::align_val_t alignment) noexcept {
    return Memory_Pool_Impl::get().allocate(size, alignment);
  }

  void Memory_Pool::release(void * const ptr, size_t size) noexcept {
    Memory_Pool_Impl::get().release(ptr, size);
  }

  void Memory_Pool::release(void * const ptr, size_t size, const std::align_val_t alignment) noexcept {
    Memory_Pool_Impl::get().release(ptr, size, alignment);
  }

}
