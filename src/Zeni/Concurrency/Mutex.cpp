#include "Zeni/Concurrency/Mutex.hpp"

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
      : m_lock(mutex.m_impl->m_mutex)
#endif
    {
    }

#ifndef DISABLE_MULTITHREADING
  private:
    std::lock_guard<std::mutex> m_lock;
#endif
  };

  Mutex::Lock::Lock(Mutex &mutex)
    : m_impl(new Mutex_Lock_Pimpl(mutex))
  {
  }

  Mutex::Lock::~Lock() {
    delete m_impl;
  }

  Mutex::Mutex()
    : m_impl(new Mutex_Pimpl)
  {
  }

  Mutex::~Mutex() {
    delete m_impl;
  }

}
