#ifndef ZENI_CONCURRENCY_MEMORY_POOL_H
#define ZENI_CONCURRENCY_MEMORY_POOL_H

#include "Mutex.hpp"

#include <cstdint>
#include <memory>

namespace Zeni::Concurrency {

  class Memory_Pool_Pimpl;

  class Memory_Pool : std::enable_shared_from_this<Memory_Pool> {
    Memory_Pool(const Memory_Pool &rhs) = delete;
    Memory_Pool & operator=(const Memory_Pool &rhs) = delete;

#ifdef NDEBUG
    static const int m_pimpl_size = 64;
#else
    static const int m_pimpl_size = 80;
#endif
    static const int m_pimpl_align = 8;
    const Memory_Pool_Pimpl * get_pimpl() const noexcept;
    Memory_Pool_Pimpl * get_pimpl() noexcept;

  public:
    ZENI_CONCURRENCY_LINKAGE Memory_Pool() noexcept;
    ZENI_CONCURRENCY_LINKAGE ~Memory_Pool() noexcept;

    /// Free any cached memory blocks. Return a count of the number of blocks freed.
    ZENI_CONCURRENCY_LINKAGE size_t clear() noexcept;

    /// Get a cached memory block or allocate one as needed.
    ZENI_CONCURRENCY_LINKAGE void * allocate(const size_t size) noexcept;

    /// Return a memory block to be cached (and eventually freed).
    ZENI_CONCURRENCY_LINKAGE void release(void * const ptr) noexcept;

    /// Get the size of a memory block (not counting header information used to store the size).
    ZENI_CONCURRENCY_LINKAGE size_t size_of(const void * const ptr) const noexcept;

  private:
    alignas(m_pimpl_align) char m_pimpl_storage[m_pimpl_size];
  };

}

#endif
