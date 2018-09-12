#ifndef ZENI_CONCURRENCY_MESSAGE_HPP
#define ZENI_CONCURRENCY_MESSAGE_HPP

#include "Job.hpp"

namespace Zeni::Concurrency {
  class Recipient;
}

namespace std {
  template class ZENI_CONCURRENCY_LINKAGE std::shared_ptr<Zeni::Concurrency::Recipient>;
}

namespace Zeni::Concurrency {

  class Recipient;

  /// An addressed message virtual base class
  class ZENI_CONCURRENCY_LINKAGE Message : public Job {
    Message(const Message &) = delete;
    Message & operator=(const Message &) = delete;

  protected:
    std::shared_ptr<const Message> shared_from_this() const noexcept;
    std::shared_ptr<Message> shared_from_this() noexcept;

  public:
    Message(const std::shared_ptr<Recipient> &recipient) noexcept;

    virtual ~Message() {}

    const std::shared_ptr<Recipient> & get_recipient() const noexcept;

    void execute() noexcept override;

  private:
    std::shared_ptr<Recipient> m_recipient;
  };

}

#endif
