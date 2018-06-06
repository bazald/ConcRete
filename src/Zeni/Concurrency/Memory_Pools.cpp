#include "Zeni/Concurrency/Memory_Pools.hpp"

#include "Zeni/Concurrency/Memory_Pool.hpp"
#include "Zeni/Concurrency/Mallocator.hpp"
#include "Zeni/Concurrency/Mutex.hpp"

#include <list>
#include <new>
#include <unordered_set>

namespace Zeni::Concurrency {

  static void new_handler() noexcept(false);

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
    static Memory_Pools_Pimpl & get() {
      static Memory_Pools_Pimpl memory_pools;
      return memory_pools;
    }

    std::shared_ptr<IMemory_Pool> get_pool() noexcept(false) {
      auto memory_pool = m_memory_pool.lock();
      if (memory_pool)
        return memory_pool;

      memory_pool = std::allocate_shared<Memory_Pool>(Mallocator<Memory_Pool>());
      m_memory_pool = memory_pool;
      m_clearer = std::allocate_shared<Clearer>(Mallocator<Clearer>());

      {
        Mutex::Lock mutex_lock(m_mutex);
        m_memory_pools.emplace(memory_pool);
      }

      return memory_pool;
    }

    void clear_pool() noexcept(false) {
      auto memory_pool = m_memory_pool.lock();
      if (memory_pool) {
        Mutex::Lock mutex_lock(m_mutex);
        m_memory_pools.erase(memory_pool);
      }
    }

    void new_handler() noexcept(false) {
      Mutex::Lock mutex_lock(m_mutex);

      if (m_memory_pools.empty())
        m_old_handler();
      else
        m_memory_pools.clear();
    }

  private:
    Mutex m_mutex;
    std::unordered_set<std::shared_ptr<Memory_Pool>,
      std::hash<std::shared_ptr<Memory_Pool>>,
      std::equal_to<std::shared_ptr<Memory_Pool>>,
      Mallocator<std::shared_ptr<Memory_Pool>>> m_memory_pools;
    std::list<std::new_handler, Mallocator<std::new_handler>> m_new_handlers;
    const std::new_handler m_old_handler;
    static thread_local std::weak_ptr<Memory_Pool> m_memory_pool;
    static thread_local std::shared_ptr<Clearer> m_clearer;
  };

  thread_local std::weak_ptr<Memory_Pool> Memory_Pools_Pimpl::m_memory_pool;
  thread_local std::shared_ptr<Memory_Pools_Pimpl::Clearer> Memory_Pools_Pimpl::m_clearer;

  void new_handler() noexcept(false) {
    Memory_Pools_Pimpl::get().new_handler();
  }

  std::shared_ptr<IMemory_Pool> Memory_Pools::get_pool() noexcept(false) {
    return Memory_Pools_Pimpl::get().get_pool();
  }

}
