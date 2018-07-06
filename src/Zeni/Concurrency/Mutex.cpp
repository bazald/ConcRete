#include "Zeni/Concurrency/Mutex.hpp"

#if ZENI_CONCURRENCY != ZENI_CONCURRENCY_NONE
#include <mutex>
#endif

namespace Zeni::Concurrency {

#if ZENI_CONCURRENCY == ZENI_CONCURRENCY_NONE
  Mutex::Lock::Lock(Mutex &) noexcept
#else
  Mutex::Lock::Lock(Mutex &mutex) noexcept
      : m_lock(mutex.m_mutex)
#endif
  {
  }

}
