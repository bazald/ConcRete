#ifndef ZENI_CONCURRENCY_WORKER_THREADS_IMPL_HPP
#define ZENI_CONCURRENCY_WORKER_THREADS_IMPL_HPP

#include "../Worker_Threads.hpp"

#ifndef DISABLE_MULTITHREADING
#include <atomic>
#include <thread>
#include <vector>
#endif

namespace Zeni::Concurrency {

  class Worker_Threads_Impl;
  void worker(Worker_Threads_Impl * const worker_threads) noexcept;

  class Worker_Threads_Impl : public Worker_Threads, public std::enable_shared_from_this<Worker_Threads_Impl> {
    Worker_Threads_Impl(const Worker_Threads_Impl &) = delete;
    Worker_Threads_Impl & operator=(const Worker_Threads_Impl &) = delete;

    /// Initialize the number of threads to std::thread::hardware_concurrency()
    Worker_Threads_Impl() noexcept(false);
    /// Initialize the number of threads to 0 or 1 for single-threaded operation, anything higher for multithreaded
    Worker_Threads_Impl(const int16_t num_threads) noexcept(false);

  public:
    /// Initialize the number of threads to std::thread::hardware_concurrency()
    static std::shared_ptr<Worker_Threads_Impl> Create() noexcept(false);
    /// Initialize the number of threads to 0 or 1 for single-threaded operation, anything higher for multithreaded
    static std::shared_ptr<Worker_Threads_Impl> Create(const int16_t num_threads) noexcept(false);

    ~Worker_Threads_Impl() noexcept;

    /// Get the total number of worker threads across all pools
    static int64_t get_total_workers() noexcept;

    std::shared_ptr<Job_Queue> get_main_Job_Queue() const noexcept override;

    void finish_jobs() noexcept(false) override;

  private:
    std::shared_ptr<Job_Queue> m_job_queue;

#ifndef DISABLE_MULTITHREADING
    void worker_awakened() noexcept override;
    void job_queue_emptied() noexcept override;
    void job_queue_nonemptied() noexcept override;

    void worker_thread_work() noexcept;

    friend void worker(Worker_Threads_Impl * const worker_threads) noexcept;
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
