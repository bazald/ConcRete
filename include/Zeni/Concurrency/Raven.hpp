#ifndef ZENI_CONCURRENCY_RAVEN_H
#define ZENI_CONCURRENCY_RAVEN_H

#include "Job.hpp"

namespace Zeni::Concurrency {

  class Maester;
  class Raven_Pimpl;

  /// An addressed message virtual base class
  class Raven : public Job {
    Raven(const Raven &) = delete;
    Raven & operator=(const Raven &) = delete;

  protected:
    ZENI_CONCURRENCY_LINKAGE std::shared_ptr<const Raven> shared_from_this() const;
    ZENI_CONCURRENCY_LINKAGE std::shared_ptr<Raven> shared_from_this();

  public:
    ZENI_CONCURRENCY_LINKAGE Raven(const std::shared_ptr<Maester> &recipient);
    ZENI_CONCURRENCY_LINKAGE ~Raven();

    ZENI_CONCURRENCY_LINKAGE const std::shared_ptr<Maester> & get_recipient() const;

    ZENI_CONCURRENCY_LINKAGE void execute(Job_Queue &job_queue) override;

  private:
    Raven_Pimpl * const m_impl;
  };

}

#endif
