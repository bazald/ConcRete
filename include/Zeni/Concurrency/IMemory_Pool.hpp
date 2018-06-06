#ifndef ZENI_CONCURRENCY_IMEMORY_POOL_HPP
#define ZENI_CONCURRENCY_IMEMORY_POOL_HPP

#include "Linkage.hpp"

#include <cstdint>
#include <memory>

namespace Zeni::Concurrency {

  class Memory_Pool;

  class IMemory_Pool {
    IMemory_Pool(const IMemory_Pool &) = delete;
    IMemory_Pool & operator=(const IMemory_Pool &) = delete;

  public:
    ZENI_CONCURRENCY_LINKAGE IMemory_Pool() = default;

    /// Free any cached memory blocks. Return a count of the number of blocks freed.
    ZENI_CONCURRENCY_LINKAGE virtual size_t clear() noexcept = 0;

    /// Get a cached memory block or allocate one as needed.
    ZENI_CONCURRENCY_LINKAGE virtual void * allocate(const size_t size) noexcept = 0;

    /// Return a memory block to be cached (and eventually freed).
    ZENI_CONCURRENCY_LINKAGE virtual void release(void * const ptr) noexcept = 0;

    /// Get the size of a memory block (not counting header information used to store the size).
    ZENI_CONCURRENCY_LINKAGE virtual size_t size_of(const void * const ptr) const noexcept = 0;
  };

}

#endif
