#ifndef ZENI_CONCURRENCY_WORKER_THREADS_HPP
#define ZENI_CONCURRENCY_WORKER_THREADS_HPP

#include "Internal/Linkage.hpp"
#include "Internal/Concurrency.hpp"

#include "Job_Queue.hpp"

#include <memory>

namespace Zeni::Concurrency {

  class Job_Queue;
  class Job_Queue_Impl;

  class Worker_Threads {
    Worker_Threads(const Worker_Threads &) = delete;
    Worker_Threads & operator=(const Worker_Threads &) = delete;

  public:
    ZENI_CONCURRENCY_LINKAGE Worker_Threads() = default;

    /// Initialize the number of threads to std::thread::hardware_concurrency()
    ZENI_CONCURRENCY_LINKAGE static std::shared_ptr<Worker_Threads> Create() noexcept(false);
    /// Initialize the number of threads to 0 or 1 for single-threaded operation, anything higher for multithreaded
    ZENI_CONCURRENCY_LINKAGE static std::shared_ptr<Worker_Threads> Create(const int16_t num_threads) noexcept(false);

    /// Get the total number of worker threads across all pools
    ZENI_CONCURRENCY_LINKAGE static int64_t get_total_workers() noexcept;

    ZENI_CONCURRENCY_LINKAGE virtual std::shared_ptr<Job_Queue> get_main_Job_Queue() const noexcept = 0;

    ZENI_CONCURRENCY_LINKAGE virtual void finish_jobs() noexcept(false) = 0;

  private:
#if ZENI_CONCURRENCY != ZENI_CONCURRENCY_NONE
    friend Job_Queue_Impl;
    virtual void worker_awakened() noexcept = 0;
    virtual void jobs_inserted(const int64_t num_jobs) noexcept = 0;
    virtual void job_removed() noexcept = 0;
#endif
  };

}

#endif
