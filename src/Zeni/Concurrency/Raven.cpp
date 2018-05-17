#include "Zeni/Concurrency/Raven.h"

#include "Zeni/Concurrency/Maester.h"

namespace Zeni {

  namespace Concurrency {

    Raven::Raven(const std::shared_ptr<Maester> &recipient_)
      : m_recipient(recipient_)
    {
    }

    Raven::Raven(std::shared_ptr<Maester> &&recipient_)
      : m_recipient(std::move(recipient_))
    {
    }

    void Raven::execute(Job_Queue &job_queue) {
      std::shared_ptr<Maester> recipient = m_recipient.lock();
      if (recipient)
        recipient->receive(job_queue, std::dynamic_pointer_cast<Raven>(shared_from_this()));
    }

  }

}
