#ifndef ZENI_CONCURRENCY_JOB_H
#define ZENI_CONCURRENCY_JOB_H

#include "Zeni/Linkage.h"

#include <memory>

namespace Zeni {

  namespace Concurrency {

    class Job;
    class Job_Queue;

  }

}

ZENI_CONCURRENCY_EXTERN template class ZENI_CONCURRENCY_LINKAGE std::weak_ptr<Zeni::Concurrency::Job>;
ZENI_CONCURRENCY_EXTERN template class ZENI_CONCURRENCY_LINKAGE std::enable_shared_from_this<Zeni::Concurrency::Job>;

namespace Zeni {

  namespace Concurrency {

    /// A Job virtual base class
    class ZENI_CONCURRENCY_LINKAGE Job : public std::enable_shared_from_this<Job> {
    public:
      /// The function that gets called by whichever worker pulls this Job off of the Job_Queue
      virtual void execute(Job_Queue &job_queue) = 0;
    };

  }

}

#endif
