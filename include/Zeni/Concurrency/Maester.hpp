#ifndef ZENI_CONCURRENCY_MAESTER_HPP
#define ZENI_CONCURRENCY_MAESTER_HPP

#include "Mutex.hpp"

#include <memory>

namespace Zeni::Concurrency {
  class Maester;
}

namespace std {
  template class ZENI_CONCURRENCY_LINKAGE std::weak_ptr<Zeni::Concurrency::Maester>;
}

namespace Zeni::Concurrency {

  class Job_Queue;
  class Raven;

  /// A message recipient virtual base class
  class ZENI_CONCURRENCY_LINKAGE Maester : public std::enable_shared_from_this<Maester> {
    Maester(const Maester &) = delete;
    Maester & operator=(const Maester &) = delete;

  protected:
    Maester() = default;

  public:
    virtual void receive(const std::shared_ptr<const Raven> raven) noexcept = 0;

  protected:
    mutable Mutex m_mutex;
  };

}

#endif
