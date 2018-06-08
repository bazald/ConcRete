#include "Zeni/Concurrency/Internal/Job_Queue_Impl.hpp"

#include "Zeni/Concurrency/Worker_Threads.hpp"

namespace Zeni::Concurrency {

  std::shared_ptr<Job_Queue> Job_Queue::Create(Worker_Threads * const worker_threads) noexcept {
    return Job_Queue_Impl::Create(worker_threads);
  }

}
