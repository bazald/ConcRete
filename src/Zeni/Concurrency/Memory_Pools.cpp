#include "Zeni/Concurrency/Memory_Pools.hpp"

#include "Zeni/Concurrency/Internal/Mallocator.hpp"
#include "Zeni/Concurrency/Internal/Memory_Pool_Impl.hpp"
#include "Zeni/Concurrency/Mutex.hpp"

#include <list>
#include <new>
#include <unordered_set>

namespace Zeni::Concurrency {

  static void new_handler() noexcept;

  class Memory_Pools_Pimpl {
    Memory_Pools_Pimpl(const Memory_Pools_Pimpl &) = delete;
    Memory_Pools_Pimpl operator=(const Memory_Pools_Pimpl &) = delete;

    class Clearer {
    public:
      ~Clearer() {
        Memory_Pools_Pimpl::get().clear_pool();
      }
    };

    Memory_Pools_Pimpl() noexcept(false)
      : m_old_handler(std::set_new_handler(Zeni::Concurrency::new_handler))
    {
    }

  public:
    static Memory_Pools_Pimpl & get() noexcept(false) {
      static Memory_Pools_Pimpl memory_pools;
      return memory_pools;
    }

    std::shared_ptr<Memory_Pool> get_pool() noexcept(false) {
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

    void clear_pool() noexcept {
      if (m_memory_pool) {
        Mutex::Lock mutex_lock(m_mutex);
        m_memory_pools.erase(m_memory_pool);
      }
      m_memory_pool.reset();
    }

    int64_t clear_pools() noexcept {
      int64_t cleared = 0;
      Mutex::Lock mutex_lock(m_mutex);
      for (auto &memory_pool : m_memory_pools)
        cleared += memory_pool->clear();
      return cleared;
    }

    void new_handler() noexcept {
      if (m_memory_pools.empty())
        m_old_handler();
      else if (!clear_pools())
        abort();
    }

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

  thread_local std::shared_ptr<Memory_Pool> Memory_Pools_Pimpl::m_memory_pool;
  thread_local std::shared_ptr<Memory_Pools_Pimpl::Clearer> Memory_Pools_Pimpl::m_clearer;

  void new_handler() noexcept {
    Memory_Pools_Pimpl::get().new_handler();
  }

  std::shared_ptr<Memory_Pool> Memory_Pools::get_pool() noexcept(false) {
    return Memory_Pools_Pimpl::get().get_pool();
  }

  void Memory_Pools::clear_pools() noexcept(false) {
    Memory_Pools_Pimpl::get().clear_pools();
  }

}
