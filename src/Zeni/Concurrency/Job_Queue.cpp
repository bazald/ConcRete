#include "Zeni/Concurrency/Internal/Job_Queue_Impl.hpp"

#include "Zeni/Concurrency/Thread_Pool.hpp"

namespace Zeni::Concurrency {

  std::shared_ptr<Job_Queue> Job_Queue::Create(Thread_Pool * const thread_pool) noexcept {
    return Job_Queue_Impl::Create(thread_pool);
  }

}
