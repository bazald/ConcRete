#ifndef ZENI_CONCURRENCY_THREAD_POOL
#define ZENI_CONCURRENCY_THREAD_POOL

#include "Job_Queue.hpp"

namespace Zeni {

  namespace Concurrency {

    class Thread_Pool_Pimpl;

    class Thread_Pool {
      Thread_Pool(const Thread_Pool &) = delete;
      Thread_Pool & operator=(const Thread_Pool &) = delete;

    public:
      /// Initialize the number of threads to std::thread::hardware_concurrency()
      ZENI_CONCURRENCY_LINKAGE Thread_Pool();
      /// Initialize the number of threads to 0 for single-threaded operation, anything else for multithreaded
      ZENI_CONCURRENCY_LINKAGE Thread_Pool(const size_t &num_threads);
      ZENI_CONCURRENCY_LINKAGE ~Thread_Pool();

      ZENI_CONCURRENCY_LINKAGE std::shared_ptr<Job_Queue> get_Job_Queue() const;

    private:
      Thread_Pool_Pimpl * const m_impl;
    };

  }

}

#endif
