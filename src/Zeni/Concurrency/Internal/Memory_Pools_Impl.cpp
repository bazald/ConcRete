#include "Zeni/Concurrency/Internal/Memory_Pools_Impl.hpp"

namespace Zeni::Concurrency {

  static void new_handler() noexcept(false);

  Memory_Pools_Impl::Clearer::~Clearer() {
    Memory_Pools_Impl::get().clear_pool();
  }

  Memory_Pools_Impl::Memory_Pools_Impl() noexcept(false)
  : m_old_handler(std::set_new_handler(Zeni::Concurrency::new_handler))
  {
  }

  Memory_Pools_Impl & Memory_Pools_Impl::get() noexcept(false) {
    static Memory_Pools_Impl memory_pools;
    return memory_pools;
  }

  std::shared_ptr<Memory_Pool> Memory_Pools_Impl::get_pool() noexcept(false) {
    if (!m_memory_pool) {
      m_memory_pool = std::allocate_shared<Memory_Pool_Impl>(Mallocator<Memory_Pool_Impl>());
      m_clearer = std::allocate_shared<Clearer>(Mallocator<Clearer>());

      {
        Mutex::Lock mutex_lock(m_mutex);
        m_memory_pools.emplace(m_memory_pool);
      }
    }

    return m_memory_pool;
  }

  void Memory_Pools_Impl::clear_pool() noexcept {
    if (m_memory_pool) {
      Mutex::Lock mutex_lock(m_mutex);
      m_memory_pools.erase(m_memory_pool);
    }
    m_memory_pool.reset();
  }

  int64_t Memory_Pools_Impl::clear_pools() noexcept {
    int64_t cleared = 0;
    Mutex::Lock mutex_lock(m_mutex);
    for (auto &memory_pool : m_memory_pools)
      cleared += memory_pool->clear();
    return cleared;
  }

  void Memory_Pools_Impl::new_handler() noexcept(false) {
    if (!clear_pools())
      m_old_handler();
  }

  thread_local std::shared_ptr<Memory_Pool> Memory_Pools_Impl::m_memory_pool;
  thread_local std::shared_ptr<Memory_Pools_Impl::Clearer> Memory_Pools_Impl::m_clearer;

  void new_handler() noexcept(false) {
    Memory_Pools_Impl::get().new_handler();
  }

}
