#ifndef ZENI_CONCURRENCY_MEMORY_POOLS_HPP
#define ZENI_CONCURRENCY_MEMORY_POOLS_HPP

#include "Internal/Linkage.hpp"

#include <cstdint>
#include <memory>

namespace Zeni::Concurrency {

  class Memory_Pool;

  class Memory_Pools {
    Memory_Pools(const Memory_Pools &) = delete;
    Memory_Pools operator=(const Memory_Pools &) = delete;

  public:
    ZENI_CONCURRENCY_LINKAGE static std::shared_ptr<Memory_Pool> get_pool() noexcept(false);

    ZENI_CONCURRENCY_LINKAGE static void clear_pools() noexcept(false);
  };

}

#endif
