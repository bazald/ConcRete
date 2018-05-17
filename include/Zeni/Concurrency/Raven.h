#ifndef ZENI_CONCURRENCY_RAVEN_H
#define ZENI_CONCURRENCY_RAVEN_H

#include "Job.h"

namespace Zeni {

  namespace Concurrency {

    class Maester;

    /// An addressed message virtual base class
    class ZENI_CONCURRENCY_LINKAGE Raven : public Job {
    public:
      Raven(const std::shared_ptr<Maester> &recipient);
      Raven(std::shared_ptr<Maester> &&recipient);

      void execute(Job_Queue &job_queue) override;

      std::weak_ptr<Maester> m_recipient;
    };

  }

}

#endif
