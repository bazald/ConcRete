#include "Zeni/Concurrency/Thread_Pool.hpp"

namespace Zeni::Concurrency {

  std::shared_ptr<IThread_Pool> IThread_Pool::Create() noexcept(false) {
    return Thread_Pool::Create();
  }

  std::shared_ptr<IThread_Pool> IThread_Pool::Create(const int16_t num_threads) noexcept(false) {
    return Thread_Pool::Create(num_threads);
  }

  int64_t IThread_Pool::get_total_workers() noexcept {
    return Thread_Pool::get_total_workers();
  }

}
