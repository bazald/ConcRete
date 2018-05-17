#ifndef ZENI_CONCURRENCY_THREAD_POOL
#define ZENI_CONCURRENCY_THREAD_POOL

#include "Job_Queue.h"
#include <list>

namespace Zeni {

  namespace Concurrency {

    class Thread_Pool {
    public:
      /// Initializes a thread pool with a number of threads equal to what is specified in the Job_Queue
      Thread_Pool(const Job_Queue::Ptr &job_queue);
      ~Thread_Pool();

      Job_Queue::Ptr get_queue() const;

    private:
      Job_Queue::Ptr m_job_queue;
      std::list<std::thread> m_workers;
    };

  }

}

#endif
