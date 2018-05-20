#ifndef ZENI_CONCURRENCY_MAESTER_H
#define ZENI_CONCURRENCY_MAESTER_H

#include "Job_Queue.hpp"
#include "Mutex.hpp"

namespace Zeni {

  namespace Concurrency {

    class Raven;

    /// A message recipient virtual base class
    class Maester {
      Maester(const Maester &) = delete;
      Maester & operator=(const Maester &) = delete;

    public:
      ZENI_CONCURRENCY_LINKAGE Maester();

      ZENI_CONCURRENCY_LINKAGE virtual void receive(Job_Queue &job_queue, const Raven &raven) = 0;

    protected:
      Mutex m_mutex;
    };

  }

}

#endif
