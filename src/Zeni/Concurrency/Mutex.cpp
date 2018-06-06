#include "Zeni/Concurrency/Mutex.hpp"

#ifndef DISABLE_MULTITHREADING
#include <mutex>
#endif

namespace Zeni::Concurrency {

#ifdef DISABLE_MULTITHREADING
  Mutex::Lock::Lock(Mutex &) noexcept
#else
  Mutex::Lock::Lock(Mutex &mutex) noexcept
      : m_lock(mutex.m_mutex)
#endif
  {
  }

}
