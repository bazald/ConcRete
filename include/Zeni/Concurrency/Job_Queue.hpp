#ifndef ZENI_CONCURRENCY_JOB_QUEUE_H
#define ZENI_CONCURRENCY_JOB_QUEUE_H

#include "Job.hpp"

#include <stdexcept>
#include <vector>

namespace Zeni::Concurrency {

  class Job_Queue_Pimpl;
  class Job_Queue_Lock_Pimpl;

  class Job_Queue {
    Job_Queue(const Job_Queue &) = delete;
    Job_Queue & operator=(const Job_Queue &) = delete;

    friend class Job_Queue_Lock_Pimpl;

  public:
    enum class ZENI_CONCURRENCY_LINKAGE Status { RUNNING, SHUTTING_DOWN, SHUT_DOWN };

    class Job_Queue_Must_Be_Running : public std::runtime_error {
    public:
      ZENI_CONCURRENCY_LINKAGE Job_Queue_Must_Be_Running();
    };

    class Job_Queue_Must_Not_Be_Shut_Down : public std::runtime_error {
    public:
      ZENI_CONCURRENCY_LINKAGE Job_Queue_Must_Not_Be_Shut_Down();
    };

    class ZENI_CONCURRENCY_LINKAGE Lock {
      Lock(const Lock &) = delete;
      Lock & operator=(const Lock &) = delete;

    public:
      Lock(Job_Queue &job_queue);
      ~Lock();

    private:
      Job_Queue_Lock_Pimpl * const m_impl;
    };

    /// Initialize the number of threads to std::thread::hardware_concurrency()
    ZENI_CONCURRENCY_LINKAGE Job_Queue();
    /// Initialize the number of threads to 0 for single-threaded operation, anything else for multithreaded
    ZENI_CONCURRENCY_LINKAGE Job_Queue(const size_t num_threads);
    ZENI_CONCURRENCY_LINKAGE ~Job_Queue();

    /// Get the number of threads.
    ZENI_CONCURRENCY_LINKAGE size_t num_threads() const;

    /// Change status to SHUTTING_DOWN and wake a thread in case all are waiting on non-empty. Can throw Job_Queue_Must_Be_Running.
    ZENI_CONCURRENCY_LINKAGE void finish();
    /// Add threads; Should be called only by Thread_Pool after threads are instantiated and before unlocking the Job_Queue.
    ZENI_CONCURRENCY_LINKAGE void add_threads(const size_t threads);

    /// Wait for the number of jobs to go to 0 and all worker threads to go dormant. i.e. quiescence
    ZENI_CONCURRENCY_LINKAGE void wait_for_completion();

    /// Take a Job off the queue. Will be null if and only if SHUT_DOWN.
    ZENI_CONCURRENCY_LINKAGE std::pair<std::shared_ptr<Job>, Status> take_one();

    /// Give the queue a new Job. Can throw Job_Queue_Must_Not_Be_Shut_Down.
    ZENI_CONCURRENCY_LINKAGE void give_one(const std::shared_ptr<Job> job);

    /// Give the queue many new Jobs. Can throw Job_Queue_Must_Not_Be_Shut_Down.
    ZENI_CONCURRENCY_LINKAGE void give_many(std::vector<std::shared_ptr<Job>> &&jobs);

  private:
    Job_Queue_Pimpl * const m_impl;
  };

}

#endif
