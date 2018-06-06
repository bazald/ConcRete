#ifndef ZENI_CONCURRENCY_MEMORY_POOL_HPP
#define ZENI_CONCURRENCY_MEMORY_POOL_HPP

#include "IMemory_Pool.hpp"
#include "Mallocator.hpp"

#include <unordered_map>

namespace Zeni::Concurrency {

  class Memory_Pool : public IMemory_Pool, public std::enable_shared_from_this<Memory_Pool> {
    Memory_Pool(const Memory_Pool &rhs) = delete;
    Memory_Pool & operator=(const Memory_Pool &rhs) = delete;

  public:
    ZENI_CONCURRENCY_LINKAGE Memory_Pool() noexcept;
    ZENI_CONCURRENCY_LINKAGE ~Memory_Pool() noexcept;

    /// Free any cached memory blocks. Return a count of the number of blocks freed.
    ZENI_CONCURRENCY_LINKAGE size_t clear() noexcept override;

    /// Get a cached memory block or allocate one as needed.
    ZENI_CONCURRENCY_LINKAGE void * allocate(const size_t size) noexcept override;

    /// Return a memory block to be cached (and eventually freed).
    ZENI_CONCURRENCY_LINKAGE void release(void * const ptr) noexcept override;

    /// Get the size of a memory block (not counting header information used to store the size).
    ZENI_CONCURRENCY_LINKAGE size_t size_of(const void * const ptr) const noexcept override;

  private:
    void Memory_Pool::fill(void * const dest, const uint32_t pattern) noexcept;

    std::unordered_map<size_t, void *, std::hash<size_t>, std::equal_to<size_t>, Mallocator<std::pair<const size_t, void *>>> m_freed;
  };

}

#endif
