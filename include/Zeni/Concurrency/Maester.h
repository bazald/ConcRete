#ifndef ZENI_CONCURRENCY_MAESTER_H
#define ZENI_CONCURRENCY_MAESTER_H

#include "Job_Queue.h"

#include <memory>
#include <mutex>

namespace Zeni {

  namespace Concurrency {

    class Raven;

    /// A message recipient virtual base class
    class Maester {
    public:
      typedef std::shared_ptr<Maester> Ptr;

      Maester(const Job_Queue::Ptr &job_queue);
      Maester(Job_Queue::Ptr &&job_queue);

      Job_Queue::Ptr get_job_queue() const;

      virtual void receive(const std::shared_ptr<Raven> &raven) = 0;

    protected:
      std::mutex m_mutex;

    private:
      Job_Queue::Ptr m_job_queue;
    };

  }

}

#endif
