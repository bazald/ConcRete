#include "Zeni/Concurrency/Raven.hpp"

#include "Zeni/Concurrency/Maester.hpp"

namespace Zeni::Concurrency {

  class Raven_Pimpl {
    Raven_Pimpl(const Raven_Pimpl &) = delete;
    Raven_Pimpl & operator=(const Raven_Pimpl &) = delete;

  public:
    Raven_Pimpl(const std::shared_ptr<Maester> &recipient_)
      : m_recipient(recipient_)
    {
    }

    const std::shared_ptr<Maester> & get_recipient() const {
      return m_recipient;
    }

    void execute(const std::shared_ptr<Raven> raven, Job_Queue &job_queue) {
      m_recipient->receive(job_queue, raven);
    }

  private:
    std::shared_ptr<Maester> m_recipient;
  };

  std::shared_ptr<const Raven> Raven::shared_from_this() const {
    return std::dynamic_pointer_cast<const Raven>(Job::shared_from_this());
  }

  std::shared_ptr<Raven> Raven::shared_from_this() {
    return std::dynamic_pointer_cast<Raven>(Job::shared_from_this());
  }

  Raven::Raven(const std::shared_ptr<Maester> &recipient_)
    : m_impl(new Raven_Pimpl(recipient_))
  {
  }

  Raven::~Raven() {
    delete m_impl;
  }

  const std::shared_ptr<Maester> & Raven::get_recipient() const {
    return m_impl->get_recipient();
  }

  void Raven::execute(Job_Queue &job_queue) {
    m_impl->execute(shared_from_this(), job_queue);
  }

}
