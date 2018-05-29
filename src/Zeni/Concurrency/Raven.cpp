#include "Zeni/Concurrency/Raven.hpp"

#include "Zeni/Concurrency/Maester.hpp"
#include "Zeni/Utility.hpp"

namespace Zeni::Concurrency {

  class Raven_Pimpl {
    Raven_Pimpl(const Raven_Pimpl &) = delete;
    Raven_Pimpl & operator=(const Raven_Pimpl &) = delete;

  public:
    Raven_Pimpl(const std::shared_ptr<Maester> &recipient_) noexcept
      : m_recipient(recipient_)
    {
    }

    const std::shared_ptr<Maester> & get_recipient() const noexcept {
      return m_recipient;
    }

    void execute(const std::shared_ptr<Raven> raven, Job_Queue &job_queue) noexcept {
      m_recipient->receive(job_queue, raven);
    }

  private:
    std::shared_ptr<Maester> m_recipient;
  };

  const Raven_Pimpl * Raven::get_pimpl() const noexcept {
    return reinterpret_cast<const Raven_Pimpl *>(m_pimpl_storage);
  }

  Raven_Pimpl * Raven::get_pimpl() noexcept {
    return reinterpret_cast<Raven_Pimpl *>(m_pimpl_storage);
  }

  std::shared_ptr<const Raven> Raven::shared_from_this() const noexcept {
    return std::dynamic_pointer_cast<const Raven>(Job::shared_from_this());
  }

  std::shared_ptr<Raven> Raven::shared_from_this() noexcept {
    return std::dynamic_pointer_cast<Raven>(Job::shared_from_this());
  }

  Raven::Raven(const std::shared_ptr<Maester> &recipient_) noexcept {
    new (&m_pimpl_storage) Raven_Pimpl(recipient_);
  }

  Raven::~Raven() noexcept {
    static_assert(std::alignment_of<Raven_Pimpl>::value <= Raven::m_pimpl_align, "Raven::m_pimpl_align is too low.");
    ZENI_STATIC_WARNING(std::alignment_of<Raven_Pimpl>::value >= Raven::m_pimpl_align, "Raven::m_pimpl_align is too high.");

    static_assert(sizeof(Raven_Pimpl) <= sizeof(Raven::m_pimpl_storage), "Raven::m_pimpl_size too low.");
    ZENI_STATIC_WARNING(sizeof(Raven_Pimpl) >= sizeof(Raven::m_pimpl_storage), "Raven::m_pimpl_size too high.");

    get_pimpl()->~Raven_Pimpl();
  }

  const std::shared_ptr<Maester> & Raven::get_recipient() const noexcept {
    return get_pimpl()->get_recipient();
  }

  void Raven::execute(Job_Queue &job_queue) noexcept {
    get_pimpl()->execute(shared_from_this(), job_queue);
  }

}
