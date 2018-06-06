#ifndef ZENI_CONCURRENCY_THREAD_POOL_HPP
#define ZENI_CONCURRENCY_THREAD_POOL_HPP

#include "../Thread_Pool.hpp"

#ifndef DISABLE_MULTITHREADING
#include <atomic>
#include <thread>
#include <vector>
#endif

namespace Zeni::Concurrency {

  class Thread_Pool_Impl;
  void worker(Thread_Pool_Impl * const thread_pool) noexcept;

  class Thread_Pool_Impl : public Thread_Pool, public std::enable_shared_from_this<Thread_Pool_Impl> {
    Thread_Pool_Impl(const Thread_Pool_Impl &) = delete;
    Thread_Pool_Impl & operator=(const Thread_Pool_Impl &) = delete;

    /// Initialize the number of threads to std::thread::hardware_concurrency()
    Thread_Pool_Impl() noexcept(false);
    /// Initialize the number of threads to 0 or 1 for single-threaded operation, anything higher for multithreaded
    Thread_Pool_Impl(const int16_t num_threads) noexcept(false);

  public:
    /// Initialize the number of threads to std::thread::hardware_concurrency()
    static std::shared_ptr<Thread_Pool_Impl> Create() noexcept(false);
    /// Initialize the number of threads to 0 or 1 for single-threaded operation, anything higher for multithreaded
    static std::shared_ptr<Thread_Pool_Impl> Create(const int16_t num_threads) noexcept(false);

    ~Thread_Pool_Impl() noexcept;

    /// Get the total number of worker threads across all pools
    static int64_t get_total_workers() noexcept;

    std::shared_ptr<Job_Queue> get_main_Job_Queue() const noexcept override;

    void finish_jobs() noexcept(false) override;

  private:
    void worker_awakened() noexcept override;
    void job_queue_emptied() noexcept override;
    void job_queue_nonemptied() noexcept override;

    void worker_thread_work() noexcept;

    std::shared_ptr<Job_Queue> m_job_queue;
#ifndef DISABLE_MULTITHREADING
    friend void worker(Thread_Pool_Impl * const thread_pool) noexcept;
    std::vector<std::shared_ptr<std::thread>> m_worker_threads;
    std::vector<std::pair<std::thread::id, std::shared_ptr<Job_Queue>>> m_job_queues;
    std::atomic_int16_t m_awake_workers = 0;
    std::atomic_int16_t m_nonempty_job_queues = 0;
    std::atomic_bool m_initialized = false;
    std::atomic<std::thread::id> m_failed_thread_id;
#endif
  };

}

#endif
