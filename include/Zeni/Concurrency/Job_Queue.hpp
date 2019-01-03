#ifndef ZENI_CONCURRENCY_JOB_QUEUE_HPP
#define ZENI_CONCURRENCY_JOB_QUEUE_HPP

#include "Internal/Linkage.hpp"

#include <memory>
#include <vector>

namespace Zeni::Concurrency {
  class Job_Queue;
}

namespace std {
  template class ZENI_CONCURRENCY_LINKAGE shared_ptr<Zeni::Concurrency::Job_Queue>;
}

namespace Zeni::Concurrency {

  class IJob;
  class Worker_Threads;

  class Job_Queue {
    Job_Queue(const Job_Queue &) = delete;
    Job_Queue & operator=(const Job_Queue &) = delete;

  public:
    ZENI_CONCURRENCY_LINKAGE Job_Queue() = default;

    ZENI_CONCURRENCY_LINKAGE virtual ~Job_Queue() {}

    ZENI_CONCURRENCY_LINKAGE static std::shared_ptr<Job_Queue> Create(Worker_Threads * const worker_threads) noexcept;

    ZENI_CONCURRENCY_LINKAGE virtual void init_next(Job_Queue * const next) noexcept = 0;

    /// Take a Job off the queue.
    ZENI_CONCURRENCY_LINKAGE virtual std::shared_ptr<IJob> try_take_one(const bool is_already_awake) noexcept = 0;

    /// Give the queue a new Job.
    ZENI_CONCURRENCY_LINKAGE virtual void give_one(const std::shared_ptr<IJob> job) noexcept(false) = 0;

    /// Give the queue many new Jobs.
    ZENI_CONCURRENCY_LINKAGE virtual void give_many(std::vector<std::shared_ptr<IJob>> &&jobs) noexcept(false) = 0;

    /// Give the queue many new Jobs.
    ZENI_CONCURRENCY_LINKAGE virtual void give_many(const std::vector<std::shared_ptr<IJob>> &jobs) noexcept(false) = 0;
  };

}

#endif
