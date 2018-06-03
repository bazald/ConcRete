#ifndef ZENI_CONCURRENCY_JOB_QUEUE_H
#define ZENI_CONCURRENCY_JOB_QUEUE_H

#include "Linkage.hpp"

#include <memory>
#include <vector>

namespace Zeni::Concurrency {

  class Job;
  class Job_Queue_Pimpl;
  class Thread_Pool;

  class Job_Queue : public std::enable_shared_from_this<Job_Queue> {
    Job_Queue(const Job_Queue &) = delete;
    Job_Queue & operator=(const Job_Queue &) = delete;

    static const int m_pimpl_size = 136;
    static const int m_pimpl_align = 8;
    const Job_Queue_Pimpl * get_pimpl() const noexcept;
    Job_Queue_Pimpl * get_pimpl() noexcept;

    ZENI_CONCURRENCY_LINKAGE Job_Queue(Thread_Pool * const thread_pool) noexcept;

  public:
    ZENI_CONCURRENCY_LINKAGE ~Job_Queue() noexcept;

    ZENI_CONCURRENCY_LINKAGE static std::shared_ptr<Job_Queue> Create(Thread_Pool * const thread_pool) noexcept;

    /// Take a Job off the queue. Will be null if and only if SHUT_DOWN.
    ZENI_CONCURRENCY_LINKAGE std::shared_ptr<Job> try_take_one(const bool is_already_awake) noexcept;

    /// Give the queue a new Job. Can throw Job_Queue_Must_Not_Be_Shut_Down.
    ZENI_CONCURRENCY_LINKAGE void give_one(const std::shared_ptr<Job> job) noexcept(false);

    /// Give the queue many new Jobs. Can throw Job_Queue_Must_Not_Be_Shut_Down.
    ZENI_CONCURRENCY_LINKAGE void give_many(std::vector<std::shared_ptr<Job>> &&jobs) noexcept(false);

    /// Give the queue many new Jobs. Can throw Job_Queue_Must_Not_Be_Shut_Down.
    ZENI_CONCURRENCY_LINKAGE void give_many(const std::vector<std::shared_ptr<Job>> &jobs) noexcept(false);

  private:
    alignas(m_pimpl_align) char m_pimpl_storage[m_pimpl_size];
  };

}

#endif
