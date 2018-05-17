#ifndef ZENI_CONCURRENCY_RAVEN_H
#define ZENI_CONCURRENCY_RAVEN_H

#include "Job.h"

namespace Zeni {

  namespace Concurrency {

    class Maester;
    class Raven_Pimpl;

    /// An addressed message virtual base class
    class ZENI_CONCURRENCY_LINKAGE Raven : public Job {
    public:
      Raven(const std::shared_ptr<Maester> &recipient);
      ~Raven();

      Maester * get_recipient() const;

      void execute(Job_Queue &job_queue) override;

    private:
      Raven_Pimpl * const m_impl;
    };

  }

}

#endif
