#include "Zeni/Concurrency/Mutex.hpp"

#include "Zeni/Utility.hpp"

#ifndef DISABLE_MULTITHREADING
#include <mutex>
#endif

namespace Zeni::Concurrency {

  class Mutex_Pimpl {
    Mutex_Pimpl(const Mutex_Pimpl &) = delete;
    Mutex_Pimpl & operator=(const Mutex_Pimpl &) = delete;

    friend class Mutex_Lock_Pimpl;

  public:
    Mutex_Pimpl()
    {
    }

#ifndef DISABLE_MULTITHREADING
  private:
    std::mutex m_mutex;
#endif
  };

  class Mutex_Lock_Pimpl {
    Mutex_Lock_Pimpl(const Mutex_Lock_Pimpl &) = delete;
    Mutex_Lock_Pimpl & operator=(const Mutex_Lock_Pimpl &) = delete;

  public:
#ifdef DISABLE_MULTITHREADING
    Mutex_Lock_Pimpl(Mutex &)
#else
    Mutex_Lock_Pimpl(Mutex &mutex)
      : m_lock(mutex.get_pimpl()->m_mutex)
#endif
    {
    }

#ifndef DISABLE_MULTITHREADING
  private:
    std::lock_guard<std::mutex> m_lock;
#endif
  };

  const Mutex_Pimpl * Mutex::get_pimpl() const {
    return reinterpret_cast<const Mutex_Pimpl *>(m_pimpl_storage);
  }

  Mutex_Pimpl * Mutex::get_pimpl() {
    return reinterpret_cast<Mutex_Pimpl *>(m_pimpl_storage);
  }

  const Mutex_Lock_Pimpl * Mutex::Lock::get_pimpl() const {
    return reinterpret_cast<const Mutex_Lock_Pimpl *>(m_pimpl_storage);
  }

  Mutex_Lock_Pimpl * Mutex::Lock::get_pimpl() {
    return reinterpret_cast<Mutex_Lock_Pimpl *>(m_pimpl_storage);
  }

  Mutex::Lock::Lock(Mutex &mutex) {
    new (&m_pimpl_storage) Mutex_Lock_Pimpl(mutex);
  }

  Mutex::Lock::~Lock() {
    static_assert(std::alignment_of<Mutex_Lock_Pimpl>::value <= Mutex::Lock::m_pimpl_align, "Job_Queue::Lock::m_pimpl_align is too low.");
    ZENI_STATIC_WARNING(std::alignment_of<Mutex_Lock_Pimpl>::value >= Mutex::Lock::m_pimpl_align, "Job_Queue::Lock::m_pimpl_align is too high.");

    static_assert(sizeof(Mutex_Lock_Pimpl) <= sizeof(Mutex::Lock::m_pimpl_storage), "Job_Queue::Lock::m_pimpl_size too low.");
    ZENI_STATIC_WARNING(sizeof(Mutex_Lock_Pimpl) >= sizeof(Mutex::Lock::m_pimpl_storage), "Job_Queue::Lock::m_pimpl_size too high.");

    get_pimpl()->~Mutex_Lock_Pimpl();
  }

  Mutex::Mutex() {
    new (&m_pimpl_storage) Mutex_Pimpl;
  }

  Mutex::~Mutex() {
    static_assert(std::alignment_of<Mutex_Pimpl>::value <= Mutex::m_pimpl_align, "Mutex::m_pimpl_align is too low.");
    ZENI_STATIC_WARNING(std::alignment_of<Mutex_Pimpl>::value >= Mutex::m_pimpl_align, "Mutex::m_pimpl_align is too high.");

    static_assert(sizeof(Mutex_Pimpl) <= sizeof(Mutex::m_pimpl_storage), "Mutex::m_pimpl_size too low.");
    ZENI_STATIC_WARNING(sizeof(Mutex_Pimpl) >= sizeof(Mutex::m_pimpl_storage), "Mutex::m_pimpl_size too high.");

    get_pimpl()->~Mutex_Pimpl();
  }

}
