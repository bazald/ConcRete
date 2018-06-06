#ifndef ZENI_CONCURRENCY_ITHREAD_POOL_HPP
#define ZENI_CONCURRENCY_ITHREAD_POOL_HPP

#include "Linkage.hpp"

#include <memory>

namespace Zeni::Concurrency {

  class IJob_Queue;
  class Job_Queue;

  class IThread_Pool {
    IThread_Pool(const IThread_Pool &) = delete;
    IThread_Pool & operator=(const IThread_Pool &) = delete;

  public:
    ZENI_CONCURRENCY_LINKAGE IThread_Pool() = default;

    /// Initialize the number of threads to std::thread::hardware_concurrency()
    ZENI_CONCURRENCY_LINKAGE static std::shared_ptr<IThread_Pool> Create() noexcept(false);
    /// Initialize the number of threads to 0 or 1 for single-threaded operation, anything higher for multithreaded
    ZENI_CONCURRENCY_LINKAGE static std::shared_ptr<IThread_Pool> Create(const int16_t num_threads) noexcept(false);

    /// Get the total number of worker threads across all pools
    ZENI_CONCURRENCY_LINKAGE static int64_t get_total_workers() noexcept;

    ZENI_CONCURRENCY_LINKAGE virtual std::shared_ptr<IJob_Queue> get_main_Job_Queue() const noexcept = 0;

    ZENI_CONCURRENCY_LINKAGE virtual void finish_jobs() noexcept(false) = 0;

  private:
    friend Job_Queue;
    virtual void worker_awakened() noexcept = 0;
    virtual void job_queue_emptied() noexcept = 0;
    virtual void job_queue_nonemptied() noexcept = 0;
  };

}

#endif
