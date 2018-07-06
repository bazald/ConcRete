#ifndef ZENI_CONCURRENCY_MUTEX_HPP
#define ZENI_CONCURRENCY_MUTEX_HPP

#include "Internal/Linkage.hpp"
#include "Internal/Concurrency.hpp"

#if ZENI_CONCURRENCY != ZENI_CONCURRENCY_NONE
#include <mutex>

namespace std {
  template class ZENI_CONCURRENCY_LINKAGE std::lock_guard<std::mutex>;
#ifdef _WIN32
  class ZENI_CONCURRENCY_LINKAGE std::mutex;
#endif
}
#endif

namespace Zeni::Concurrency {
  
  class ZENI_CONCURRENCY_LINKAGE Mutex {
    Mutex(const Mutex &) = delete;
    Mutex & operator=(const Mutex &) = delete;

  public:
    class ZENI_CONCURRENCY_LINKAGE Lock {
      Lock(const Lock &) = delete;
      Lock & operator=(const Lock &) = delete;

    public:
      Lock(Mutex &mutex) noexcept;

    private:
#if ZENI_CONCURRENCY != ZENI_CONCURRENCY_NONE
      std::lock_guard<std::mutex> m_lock;
#endif
    };

    Mutex() noexcept = default;

  private:
#if ZENI_CONCURRENCY != ZENI_CONCURRENCY_NONE
    std::mutex m_mutex;
#endif
  };

}

#endif
