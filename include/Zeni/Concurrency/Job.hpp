#ifndef ZENI_CONCURRENCY_JOB_H
#define ZENI_CONCURRENCY_JOB_H

#include "Linkage.hpp"

#include <memory>

namespace Zeni {

  namespace Concurrency {

    class Job_Queue;

    /// A Job virtual base class
    class Job : public std::enable_shared_from_this<Job> {
    public:
      /// The function that gets called by whichever worker pulls this Job off of the Job_Queue
      ZENI_CONCURRENCY_LINKAGE virtual void execute(Job_Queue &job_queue) = 0;
    };

  }

}

#endif
