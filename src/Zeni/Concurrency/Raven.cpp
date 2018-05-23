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

      Maester * get_recipient() const {
        return m_recipient.get();
      }

      void execute(Raven * const &raven, Job_Queue &job_queue) {
        m_recipient->receive(job_queue, *raven);
      }

    private:
      std::shared_ptr<Maester> m_recipient;
    };

    Raven::Raven(const std::shared_ptr<Maester> &recipient_)
      : m_impl(new Raven_Pimpl(recipient_))
    {
    }

    Raven::~Raven() {
      delete m_impl;
    }

    Maester * Raven::get_recipient() const {
      return m_impl->get_recipient();
    }

    void Raven::execute(Job_Queue &job_queue) {
      m_impl->execute(this, job_queue);
    }

  }

}
