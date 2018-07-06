#ifndef ZENI_CONCURRENCY_JOB_QUEUE_IMPL_HPP
#define ZENI_CONCURRENCY_JOB_QUEUE_IMPL_HPP

#include "../Job_Queue.hpp"
#include "../Container/Queue.hpp"

#if ZENI_CONCURRENCY != ZENI_CONCURRENCY_NONE
#include <atomic>
#endif

namespace Zeni::Concurrency {

  class Job_Queue_Impl : public Job_Queue, public std::enable_shared_from_this<Job_Queue> {
    Job_Queue_Impl(const Job_Queue_Impl &) = delete;
    Job_Queue_Impl & operator=(const Job_Queue_Impl &) = delete;

    Job_Queue_Impl(Worker_Threads * const worker_threads) noexcept;

  public:
    static std::shared_ptr<Job_Queue_Impl> Create(Worker_Threads * const worker_threads) noexcept;

    /// Take a Job off the queue.
    std::shared_ptr<IJob> try_take_one(const bool is_already_awake) noexcept override;

    /// Test for reclaim and set to false if true.
    bool try_reclaim() noexcept override;

    /// Give the queue a new Job.
    void give_one(const std::shared_ptr<IJob> job) noexcept(false) override;

    /// Give the queue many new Jobs.
    void give_many(std::vector<std::shared_ptr<IJob>> &&jobs) noexcept(false) override;

    /// Give the queue many new Jobs.
    void give_many(const std::vector<std::shared_ptr<IJob>> &jobs) noexcept(false) override;

    /// Set reclaim to true.
    void set_reclaim() noexcept override;

  private:
#if ZENI_CONCURRENCY != ZENI_CONCURRENCY_NONE
    std::atomic_bool m_reclaim = false;
#endif
    Worker_Threads * const m_worker_threads;
    Queue<std::shared_ptr<IJob>> m_jobs;
  };

}

#endif
