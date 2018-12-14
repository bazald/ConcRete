#ifndef ZENI_CONCURRENCY_MEMORY_POOL_HPP
#define ZENI_CONCURRENCY_MEMORY_POOL_HPP

#include "Internal/Linkage.hpp"

#include <cstdint>
#include <memory>

namespace Zeni::Concurrency {

  typedef std::pair<size_t, std::align_val_t> Memory_Pool_Key;

  class Memory_Pool_Impl;

  class Memory_Pool {
  public:
    /// Free any cached memory blocks.
    ZENI_CONCURRENCY_LINKAGE static void clear() noexcept;

    /// Free any cached memory blocks from the last time we were in the current epoch.
    ZENI_CONCURRENCY_LINKAGE static void rotate() noexcept;

    /// Get a cached memory block or allocate one as needed.
    ZENI_CONCURRENCY_LINKAGE static void * allocate(size_t size) noexcept;

    /// Get a cached memory block or allocate one as needed.
    ZENI_CONCURRENCY_LINKAGE static void * allocate(size_t size, const std::align_val_t alignment) noexcept;

    /// Return a memory block to be cached (and eventually freed).
    ZENI_CONCURRENCY_LINKAGE static void release(void * const ptr, size_t size) noexcept;

    /// Return a memory block to be cached (and eventually freed).
    ZENI_CONCURRENCY_LINKAGE static void release(void * const ptr, size_t size, const std::align_val_t alignment) noexcept;
  };

}

#endif
