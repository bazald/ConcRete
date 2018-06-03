#ifndef ZENI_CONCURRENCY_THREAD_POOL_HPP
#define ZENI_CONCURRENCY_THREAD_POOL_HPP

#include "Linkage.hpp"

#include <memory>

namespace Zeni::Concurrency {

  class Job_Queue;
  class Job_Queue_Pimpl;
  class Thread_Pool_Pimpl;

  class Thread_Pool : public std::enable_shared_from_this<Thread_Pool> {
    Thread_Pool(const Thread_Pool &) = delete;
    Thread_Pool & operator=(const Thread_Pool &) = delete;

#ifdef NDEBUG
    static const int m_pimpl_size = 80;
#else
    static const int m_pimpl_size = 96;
#endif
    static const int m_pimpl_align = 8;
    const Thread_Pool_Pimpl * get_pimpl() const noexcept;
    Thread_Pool_Pimpl * get_pimpl() noexcept;

  public:
    /// Initialize the number of threads to std::thread::hardware_concurrency()
    ZENI_CONCURRENCY_LINKAGE Thread_Pool() noexcept(false);
    /// Initialize the number of threads to 0 for single-threaded operation, anything else for multithreaded
    ZENI_CONCURRENCY_LINKAGE Thread_Pool(const int16_t num_threads) noexcept(false);
    ZENI_CONCURRENCY_LINKAGE ~Thread_Pool() noexcept;

    ZENI_CONCURRENCY_LINKAGE std::shared_ptr<Job_Queue> get_main_Job_Queue() const noexcept;

    ZENI_CONCURRENCY_LINKAGE void finish_jobs() noexcept(false);

  private:
    friend Job_Queue_Pimpl;
    ZENI_CONCURRENCY_LINKAGE void worker_awakened() noexcept;
    ZENI_CONCURRENCY_LINKAGE void job_queue_emptied() noexcept;
    ZENI_CONCURRENCY_LINKAGE void job_queue_nonemptied() noexcept;

    alignas(m_pimpl_align) char m_pimpl_storage[m_pimpl_size];
  };

}

#endif
