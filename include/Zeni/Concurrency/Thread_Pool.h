#ifndef ZENI_CONCURRENCY_THREAD_POOL
#define ZENI_CONCURRENCY_THREAD_POOL

#include "Job_Queue.h"

namespace Zeni {

  namespace Concurrency {

    class Thread_Pool_Pimpl;

    class ZENI_CONCURRENCY_LINKAGE Thread_Pool {
      Thread_Pool(const Thread_Pool &) = delete;
      Thread_Pool & operator=(const Thread_Pool &) = delete;

    public:
      /// Initializes a thread pool with a number of threads equal to what is specified in the Job_Queue
      Thread_Pool(const std::shared_ptr<Job_Queue> &job_queue);
      ~Thread_Pool();

      std::shared_ptr<Job_Queue> get_queue() const;

    private:
      Thread_Pool_Pimpl * const m_impl;
    };

  }

}

#endif
