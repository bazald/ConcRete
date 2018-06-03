#ifndef ZENI_CONCURRENCY_MEMORY_POOLS_HPP
#define ZENI_CONCURRENCY_MEMORY_POOLS_HPP

#include "Mutex.hpp"

#include <cstdint>
#include <memory>

namespace Zeni::Concurrency {

  class Memory_Pool;

  class Memory_Pools {
    Memory_Pools(const Memory_Pools &) = delete;
    Memory_Pools operator=(const Memory_Pools &) = delete;

  public:
    static ZENI_CONCURRENCY_LINKAGE std::shared_ptr<Memory_Pool> get_pool() noexcept(false);
  };

}

#endif
