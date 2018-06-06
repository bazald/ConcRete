#ifndef ZENI_CONCURRENCY_JOB_QUEUE_HPP
#define ZENI_CONCURRENCY_JOB_QUEUE_HPP

#include "../Job_Queue.hpp"

#include <queue>

#ifndef DISABLE_MULTITHREADING
#include <atomic>
#include <mutex>
#endif

namespace Zeni::Concurrency {

  class Job_Queue_Impl : public Job_Queue, public std::enable_shared_from_this<Job_Queue> {
    Job_Queue_Impl(const Job_Queue_Impl &) = delete;
    Job_Queue_Impl & operator=(const Job_Queue_Impl &) = delete;

    Job_Queue_Impl(Thread_Pool * const thread_pool) noexcept;

  public:
    static std::shared_ptr<Job_Queue_Impl> Create(Thread_Pool * const thread_pool) noexcept;

    /// Take a Job off the queue. Will be null if and only if SHUT_DOWN.
    std::shared_ptr<IJob> try_take_one(const bool is_already_awake) noexcept override;

    /// Give the queue a new Job. Can throw Job_Queue_Must_Not_Be_Shut_Down.
    void give_one(const std::shared_ptr<IJob> job) noexcept(false) override;

    /// Give the queue many new Jobs. Can throw Job_Queue_Must_Not_Be_Shut_Down.
    void give_many(std::vector<std::shared_ptr<IJob>> &&jobs) noexcept(false) override;

    /// Give the queue many new Jobs. Can throw Job_Queue_Must_Not_Be_Shut_Down.
    void give_many(const std::vector<std::shared_ptr<IJob>> &jobs) noexcept(false) override;

  private:
#ifndef DISABLE_MULTITHREADING
    std::atomic_bool m_has_jobs = false;
    std::mutex m_mutex;
#endif
    Thread_Pool * const m_thread_pool;
    std::queue<std::vector<std::shared_ptr<IJob>>> m_jobs;
  };

}

#endif
