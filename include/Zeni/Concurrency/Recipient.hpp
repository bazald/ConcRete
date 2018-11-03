#ifndef ZENI_CONCURRENCY_RECIPIENT_HPP
#define ZENI_CONCURRENCY_RECIPIENT_HPP

#include "Mutex.hpp"

#include <memory>

namespace Zeni::Concurrency {
  class Recipient;
}

namespace std {
  template class ZENI_CONCURRENCY_LINKAGE std::weak_ptr<Zeni::Concurrency::Recipient>;
}

namespace Zeni::Concurrency {

  class Job_Queue;
  class Message;

  /// A message recipient virtual base class
  class ZENI_CONCURRENCY_LINKAGE Recipient : public std::enable_shared_from_this<Recipient> {
    Recipient(const Recipient &) = delete;
    Recipient & operator=(const Recipient &) = delete;

  protected:
    Recipient() = default;

  public:
    virtual ~Recipient() {}

    virtual void receive(const std::shared_ptr<const Message> message) noexcept = 0;

#if ZENI_CONCURRENCY == ZENI_CONCURRENCY_LOCKING
  protected:
    mutable Mutex m_mutex;
#endif
  };

}

#endif
