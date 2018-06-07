#ifndef ZENI_CONCURRENCY_MEMORY_POOLS_IMPL_HPP
#define ZENI_CONCURRENCY_MEMORY_POOLS_IMPL_HPP

#include "Mallocator.hpp"
#include "Memory_Pool_Impl.hpp"
#include "../Memory_Pools.hpp"
#include "../Mutex.hpp"

#include <cstdint>
#include <list>
#include <unordered_set>

namespace Zeni::Concurrency {

  class Memory_Pools_Impl {
    Memory_Pools_Impl(const Memory_Pools_Impl &) = delete;
    Memory_Pools_Impl operator=(const Memory_Pools_Impl &) = delete;

    class Clearer {
    public:
      ~Clearer();
    };

    Memory_Pools_Impl() noexcept(false);

  public:
    static Memory_Pools_Impl & get() noexcept(false);

    std::shared_ptr<Memory_Pool> get_pool() noexcept(false);

    void clear_pool() noexcept;

    int64_t clear_pools() noexcept;

    void new_handler() noexcept(false);

  private:
    Mutex m_mutex;
    std::unordered_set<std::shared_ptr<Memory_Pool>,
      std::hash<std::shared_ptr<Memory_Pool>>,
      std::equal_to<std::shared_ptr<Memory_Pool>>,
      Mallocator<std::shared_ptr<Memory_Pool>>> m_memory_pools;
    std::list<std::new_handler, Mallocator<std::new_handler>> m_new_handlers;
    const std::new_handler m_old_handler;
    static thread_local std::shared_ptr<Memory_Pool> m_memory_pool;
    static thread_local std::shared_ptr<Clearer> m_clearer;
  };

}

#endif
