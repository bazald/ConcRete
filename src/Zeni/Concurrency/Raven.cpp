#include "Zeni/Concurrency/Raven.hpp"

#include "Zeni/Concurrency/Maester.hpp"
#include "Zeni/Utility.hpp"

namespace Zeni::Concurrency {

  std::shared_ptr<const Raven> Raven::shared_from_this() const noexcept {
    return std::static_pointer_cast<const Raven>(Job::shared_from_this());
  }

  std::shared_ptr<Raven> Raven::shared_from_this() noexcept {
    return std::static_pointer_cast<Raven>(Job::shared_from_this());
  }

  Raven::Raven(const std::shared_ptr<Maester> &recipient_) noexcept
    : m_recipient(recipient_)
  {
  }

  const std::shared_ptr<Maester> & Raven::get_recipient() const noexcept {
    return m_recipient;
  }

  void Raven::execute() noexcept {
    m_recipient->receive(shared_from_this());
  }

}
