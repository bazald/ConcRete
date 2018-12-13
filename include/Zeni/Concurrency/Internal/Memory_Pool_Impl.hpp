#ifndef ZENI_CONCURRENCY_MEMORY_POOL_IMPL_HPP
#define ZENI_CONCURRENCY_MEMORY_POOL_IMPL_HPP

#include "../Memory_Pool.hpp"
#include "Mallocator.hpp"

#include <mutex>
#include <unordered_map>

namespace Zeni::Concurrency {

  class Memory_Pool_Impl : public Memory_Pool, public std::enable_shared_from_this<Memory_Pool_Impl> {
    Memory_Pool_Impl(const Memory_Pool_Impl &rhs) = delete;
    Memory_Pool_Impl & operator=(const Memory_Pool_Impl &rhs) = delete;

  public:
    Memory_Pool_Impl() noexcept;
    ~Memory_Pool_Impl() noexcept;

    /// Free any cached memory blocks. Return a count of the number of blocks freed.
    size_t clear() noexcept override;

    /// Get a cached memory block or allocate one as needed.
    void * allocate(const size_t size) noexcept override;

    /// Get a cached memory block or allocate one as needed.
    void * allocate(const size_t size, const std::align_val_t alignment) noexcept override;

    /// Return a memory block to be cached (and eventually freed).
    void release(void * const ptr) noexcept override;

    /// Return a memory block to be cached (and eventually freed).
    void release(void * const ptr, const std::align_val_t alignment) noexcept override;

    /// Get the size of a memory block (not counting header information used to store the size).
    size_t size_of(const void * const ptr) const noexcept override;

  private:
    void fill(void * const dest, const uint32_t pattern) noexcept;

    std::unordered_map<size_t, void *, std::hash<size_t>, std::equal_to<size_t>, Jemallocator<std::pair<const size_t, void *>>> m_freed;
  };

}

#endif
