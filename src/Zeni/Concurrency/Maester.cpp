#include "Zeni/Concurrency/Maester.h"

#include "Zeni/Concurrency/Raven.h"

namespace Zeni {

  namespace Concurrency {

    Maester::Maester(const std::shared_ptr<Job_Queue> &job_queue)
      : m_job_queue(job_queue)
    {
    }

    Maester::Maester(std::shared_ptr<Job_Queue> &&job_queue)
      : m_job_queue(std::move(job_queue))
    {
    }

    Job_Queue & Maester::get_job_queue() const {
      return *m_job_queue;
    }

  }

}
