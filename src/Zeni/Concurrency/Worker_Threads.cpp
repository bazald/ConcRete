#include "Zeni/Concurrency/Internal/Worker_Threads_Impl.hpp"

namespace Zeni::Concurrency {

  std::shared_ptr<Worker_Threads> Worker_Threads::Create() noexcept(false) {
    return Worker_Threads_Impl::Create();
  }

  std::shared_ptr<Worker_Threads> Worker_Threads::Create(const int16_t num_threads) noexcept(false) {
    return Worker_Threads_Impl::Create(num_threads);
  }

  int64_t Worker_Threads::get_total_workers() noexcept {
    return Worker_Threads_Impl::get_total_workers();
  }

}
