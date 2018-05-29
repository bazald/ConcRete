#ifndef ZENI_CONCURRENCY_THREAD_POOL
#define ZENI_CONCURRENCY_THREAD_POOL

#include "Linkage.hpp"

#include <memory>

namespace Zeni::Concurrency {

  class Job_Queue;
  class Thread_Pool_Pimpl;

  class Thread_Pool : public std::enable_shared_from_this<Thread_Pool> {
    Thread_Pool(const Thread_Pool &) = delete;
    Thread_Pool & operator=(const Thread_Pool &) = delete;

#ifdef NDEBUG
    static const int m_pimpl_size = 32;
#else
    static const int m_pimpl_size = 40;
#endif
    static const int m_pimpl_align = 8;
    const Thread_Pool_Pimpl * get_pimpl() const noexcept;
    Thread_Pool_Pimpl * get_pimpl() noexcept;

  public:
    /// Initialize the number of threads to std::thread::hardware_concurrency()
    ZENI_CONCURRENCY_LINKAGE Thread_Pool() noexcept;
    /// Initialize the number of threads to 0 for single-threaded operation, anything else for multithreaded
    ZENI_CONCURRENCY_LINKAGE Thread_Pool(const size_t num_threads) noexcept;
    ZENI_CONCURRENCY_LINKAGE ~Thread_Pool() noexcept;

    ZENI_CONCURRENCY_LINKAGE std::shared_ptr<Job_Queue> get_Job_Queue() const noexcept;

  private:
    alignas(m_pimpl_align) char m_pimpl_storage[m_pimpl_size];
  };

}

#endif
