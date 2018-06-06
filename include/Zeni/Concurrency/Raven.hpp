#ifndef ZENI_CONCURRENCY_RAVEN_HPP
#define ZENI_CONCURRENCY_RAVEN_HPP

#include "Job.hpp"

namespace Zeni::Concurrency {

  class Maester;

  /// An addressed message virtual base class
  class Raven : public Job {
    Raven(const Raven &) = delete;
    Raven & operator=(const Raven &) = delete;

  protected:
    std::shared_ptr<const Raven> shared_from_this() const noexcept;
    std::shared_ptr<Raven> shared_from_this() noexcept;

  public:
    ZENI_CONCURRENCY_LINKAGE Raven(const std::shared_ptr<Maester> &recipient) noexcept;

    ZENI_CONCURRENCY_LINKAGE const std::shared_ptr<Maester> & get_recipient() const noexcept;

    ZENI_CONCURRENCY_LINKAGE void execute() noexcept override;

  private:
    std::shared_ptr<Maester> m_recipient;
  };

}

#endif
