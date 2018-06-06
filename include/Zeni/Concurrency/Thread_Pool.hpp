#ifndef ZENI_CONCURRENCY_ITHREAD_POOL_HPP
#define ZENI_CONCURRENCY_ITHREAD_POOL_HPP

#include "Internal/Linkage.hpp"

#include <memory>

namespace Zeni::Concurrency {

  class Job_Queue;
  class Job_Queue_Impl;

  class Thread_Pool {
    Thread_Pool(const Thread_Pool &) = delete;
    Thread_Pool & operator=(const Thread_Pool &) = delete;

  public:
    ZENI_CONCURRENCY_LINKAGE Thread_Pool() = default;

    /// Initialize the number of threads to std::thread::hardware_concurrency()
    ZENI_CONCURRENCY_LINKAGE static std::shared_ptr<Thread_Pool> Create() noexcept(false);
    /// Initialize the number of threads to 0 or 1 for single-threaded operation, anything higher for multithreaded
    ZENI_CONCURRENCY_LINKAGE static std::shared_ptr<Thread_Pool> Create(const int16_t num_threads) noexcept(false);

    /// Get the total number of worker threads across all pools
    ZENI_CONCURRENCY_LINKAGE static int64_t get_total_workers() noexcept;

    ZENI_CONCURRENCY_LINKAGE virtual std::shared_ptr<Job_Queue> get_main_Job_Queue() const noexcept = 0;

    ZENI_CONCURRENCY_LINKAGE virtual void finish_jobs() noexcept(false) = 0;

  private:
    friend Job_Queue_Impl;
    virtual void worker_awakened() noexcept = 0;
    virtual void job_queue_emptied() noexcept = 0;
    virtual void job_queue_nonemptied() noexcept = 0;
  };

}

#endif
