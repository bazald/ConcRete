#ifndef ZENI_CONCURRENCY_IMEMORY_POOL_HPP
#define ZENI_CONCURRENCY_IMEMORY_POOL_HPP

#include "Internal/Linkage.hpp"

#include <cstdint>
#include <memory>

namespace Zeni::Concurrency {

  class Memory_Pool {
    Memory_Pool(const Memory_Pool &) = delete;
    Memory_Pool & operator=(const Memory_Pool &) = delete;

  protected:
    ZENI_CONCURRENCY_LINKAGE Memory_Pool() = default;

  public:
    ZENI_CONCURRENCY_LINKAGE static std::shared_ptr<Memory_Pool> Create() noexcept(false);

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
