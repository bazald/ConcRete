#ifndef ZENI_CONCURRENCY_MAESTER_H
#define ZENI_CONCURRENCY_MAESTER_H

#include "Mutex.hpp"

#include <memory>

namespace Zeni::Concurrency {

  class Job_Queue;
  class Raven;

  /// A message recipient virtual base class
  class Maester : public std::enable_shared_from_this<Maester> {
    Maester(const Maester &) = delete;
    Maester & operator=(const Maester &) = delete;

  protected:
    ZENI_CONCURRENCY_LINKAGE Maester() noexcept;

  public:
    ZENI_CONCURRENCY_LINKAGE virtual void receive(Job_Queue &job_queue, const std::shared_ptr<const Raven> raven) noexcept = 0;

  protected:
    mutable Mutex m_mutex;
  };

}

#endif
