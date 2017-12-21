#include "raven.h"

#include "maester.h"

Raven::Raven(const Maester::Ptr &recipient_)
  : m_recipient(recipient_)
{
}

Raven::Raven(Maester::Ptr &&recipient_)
  : m_recipient(std::move(recipient_))
{
}

void Raven::execute() {
  std::shared_ptr<Maester> recipient = m_recipient.lock();
  if(recipient)
    recipient->receive(std::dynamic_pointer_cast<Raven>(shared_from_this()));
}
