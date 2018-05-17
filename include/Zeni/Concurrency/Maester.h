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
      Maester();

      virtual void receive(Job_Queue &job_queue, const std::shared_ptr<Raven> &raven) = 0;

    protected:
      std::mutex m_mutex;
    };

  }

}

#endif
