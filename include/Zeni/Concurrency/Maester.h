#ifndef ZENI_CONCURRENCY_MAESTER_H
#define ZENI_CONCURRENCY_MAESTER_H

#include "Job_Queue.h"
#include "Mutex.h"

namespace Zeni {

  namespace Concurrency {

    class Raven;

    /// A message recipient virtual base class
    class ZENI_CONCURRENCY_LINKAGE Maester {
      Maester(const Maester &) = delete;
      Maester & operator=(const Maester &) = delete;

    public:
      Maester();

      virtual void receive(Job_Queue &job_queue, const Raven &raven) = 0;

    protected:
      Mutex m_mutex;
    };

  }

}

#endif
