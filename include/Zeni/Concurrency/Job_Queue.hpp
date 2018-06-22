#ifndef ZENI_CONCURRENCY_JOB_QUEUE_HPP
#define ZENI_CONCURRENCY_JOB_QUEUE_HPP

#include "Internal/Linkage.hpp"

#include <memory>
#include <vector>

namespace Zeni::Concurrency {
  class Job_Queue;
}

namespace std {
  template class ZENI_CONCURRENCY_LINKAGE std::shared_ptr<Zeni::Concurrency::Job_Queue>;
}

namespace Zeni::Concurrency {

  class IJob;
  class Worker_Threads;

  class Job_Queue {
    Job_Queue(const Job_Queue &) = delete;
    Job_Queue & operator=(const Job_Queue &) = delete;

  public:
    ZENI_CONCURRENCY_LINKAGE Job_Queue() = default;

    ZENI_CONCURRENCY_LINKAGE static std::shared_ptr<Job_Queue> Create(Worker_Threads * const worker_threads) noexcept;

    /// Take a Job off the queue.
    ZENI_CONCURRENCY_LINKAGE virtual std::shared_ptr<IJob> try_take_one(const bool is_already_awake) noexcept = 0;

    /// Test for reclaim and set to false if true.
    ZENI_CONCURRENCY_LINKAGE virtual bool try_reclaim() noexcept = 0;

    /// Give the queue a new Job.
    ZENI_CONCURRENCY_LINKAGE virtual void give_one(const std::shared_ptr<IJob> job) noexcept(false) = 0;

    /// Give the queue many new Jobs.
    ZENI_CONCURRENCY_LINKAGE virtual void give_many(std::vector<std::shared_ptr<IJob>> &&jobs) noexcept(false) = 0;

    /// Give the queue many new Jobs.
    ZENI_CONCURRENCY_LINKAGE virtual void give_many(const std::vector<std::shared_ptr<IJob>> &jobs) noexcept(false) = 0;

    /// Set reclaim to true.
    ZENI_CONCURRENCY_LINKAGE virtual void set_reclaim() noexcept = 0;
  };

}

#endif
