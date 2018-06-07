#include "Zeni/Concurrency/Internal/Memory_Pools_Impl.hpp"

namespace Zeni::Concurrency {

  std::shared_ptr<Memory_Pool> Memory_Pools::get_pool() noexcept(false) {
    return Memory_Pools_Impl::get().get_pool();
  }

  void Memory_Pools::clear_pools() noexcept(false) {
    Memory_Pools_Impl::get().clear_pools();
  }

}
