#include "Zeni/Concurrency/Internal/Thread_Pool_Impl.hpp"

namespace Zeni::Concurrency {

  std::shared_ptr<Thread_Pool> Thread_Pool::Create() noexcept(false) {
    return Thread_Pool_Impl::Create();
  }

  std::shared_ptr<Thread_Pool> Thread_Pool::Create(const int16_t num_threads) noexcept(false) {
    return Thread_Pool_Impl::Create(num_threads);
  }

  int64_t Thread_Pool::get_total_workers() noexcept {
    return Thread_Pool_Impl::get_total_workers();
  }

}
