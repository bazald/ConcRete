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

    static const int m_pimpl_size = 16;
    static const int m_pimpl_align = 8;
    const Raven_Pimpl * get_pimpl() const;
    Raven_Pimpl * get_pimpl();

  protected:
    ZENI_CONCURRENCY_LINKAGE std::shared_ptr<const Raven> shared_from_this() const;
    ZENI_CONCURRENCY_LINKAGE std::shared_ptr<Raven> shared_from_this();

  public:
    ZENI_CONCURRENCY_LINKAGE Raven(const std::shared_ptr<Maester> &recipient);
    ZENI_CONCURRENCY_LINKAGE ~Raven();

    ZENI_CONCURRENCY_LINKAGE const std::shared_ptr<Maester> & get_recipient() const;

    ZENI_CONCURRENCY_LINKAGE void execute(Job_Queue &job_queue) override;

  private:
    alignas(m_pimpl_align) char m_pimpl_storage[m_pimpl_size];
  };

}

#endif
