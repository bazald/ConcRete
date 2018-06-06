#include "Zeni/Concurrency/Job_Queue.hpp"

#include "Zeni/Concurrency/Thread_Pool.hpp"

namespace Zeni::Concurrency {

  std::shared_ptr<IJob_Queue> IJob_Queue::Create(Thread_Pool * const thread_pool) noexcept {
    return Job_Queue::Create(thread_pool);
  }

}
