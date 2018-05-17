#ifndef ZENI_CONCURRENCY_RAVEN_H
#define ZENI_CONCURRENCY_RAVEN_H

#include "Job.h"

namespace Zeni {

  namespace Concurrency {

    class Maester;

    /// An addressed message virtual base class
    class Raven : public Job {
    public:
      typedef std::shared_ptr<Raven> Ptr;

      Raven(const std::shared_ptr<Maester> &recipient);
      Raven(std::shared_ptr<Maester> &&recipient);

      void execute() override;

      std::weak_ptr<Maester> m_recipient;
    };

  }

}

#endif
