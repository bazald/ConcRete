#include "Zeni/Concurrency/Message.hpp"

#include "Zeni/Concurrency/Recipient.hpp"
#include "Zeni/Utility.hpp"

namespace Zeni::Concurrency {

  std::shared_ptr<const Message> Message::shared_from_this() const noexcept {
    return std::static_pointer_cast<const Message>(Job::shared_from_this());
  }

  std::shared_ptr<Message> Message::shared_from_this() noexcept {
    return std::static_pointer_cast<Message>(Job::shared_from_this());
  }

  Message::Message(const std::shared_ptr<Recipient> &recipient_) noexcept
    : m_recipient(recipient_)
  {
  }

  const std::shared_ptr<Recipient> & Message::get_recipient() const noexcept {
    return m_recipient;
  }

  void Message::execute() noexcept {
    m_recipient->receive(shared_from_this());
  }

}
