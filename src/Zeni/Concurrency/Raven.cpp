#include "Zeni/Concurrency/Raven.hpp"

#include "Zeni/Concurrency/Maester.hpp"

namespace Zeni {

  namespace Concurrency {

    class Raven_Pimpl {
      Raven_Pimpl(const Raven_Pimpl &) = delete;
      Raven_Pimpl & operator=(const Raven_Pimpl &) = delete;

    public:
      Raven_Pimpl(const std::shared_ptr<Maester> &recipient_)
        : m_recipient(recipient_)
      {
      }

      std::weak_ptr<Maester> get_recipient() const {
        return m_recipient;
      }

    private:
      std::weak_ptr<Maester> m_recipient;
    };

    Raven::Raven(const std::shared_ptr<Maester> &recipient_)
      : m_impl(new Raven_Pimpl(recipient_))
    {
    }

    Raven::~Raven() {
      delete m_impl;
    }

    Maester * Raven::get_recipient() const {
      return m_impl->get_recipient().lock().get();
    }

    void Raven::execute(Job_Queue &job_queue) {
      std::shared_ptr<Maester> recipient = m_impl->get_recipient().lock();
      if (recipient)
        recipient->receive(job_queue, *this);
    }

  }

}
