#ifndef ZENI_CONCURRENCY_MEMORY_POOLS_HPP
#define ZENI_CONCURRENCY_MEMORY_POOLS_HPP

#include "Linkage.hpp"

#include <cstdint>
#include <memory>

namespace Zeni::Concurrency {

  class IMemory_Pool;

  class Memory_Pools {
    Memory_Pools(const Memory_Pools &) = delete;
    Memory_Pools operator=(const Memory_Pools &) = delete;

  public:
    static ZENI_CONCURRENCY_LINKAGE std::shared_ptr<IMemory_Pool> get_pool() noexcept(false);
  };

}

#endif
