#ifndef ZENI_CONCURRENCY_MAESTER_H
#define ZENI_CONCURRENCY_MAESTER_H

#include "Job_Queue.h"

#include <mutex>

namespace Zeni {

  namespace Concurrency {

    class Raven;

    /// A message recipient virtual base class
    class ZENI_CONCURRENCY_LINKAGE Maester {
      Maester(const Maester &) = delete;
      Maester & operator=(const Maester &) = delete;

    public:
      typedef std::shared_ptr<Maester> Ptr;

      Maester(const std::shared_ptr<Job_Queue> &job_queue);
      Maester(std::shared_ptr<Job_Queue> &&job_queue);

      Job_Queue & get_job_queue() const;

      virtual void receive(const std::shared_ptr<Raven> &raven) = 0;

    protected:
      std::mutex m_mutex;

    private:
      std::shared_ptr<Job_Queue> m_job_queue;
    };

  }

}

#endif
