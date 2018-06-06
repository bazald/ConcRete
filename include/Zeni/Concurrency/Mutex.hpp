#ifndef ZENI_CONCURRENCY_MUTEX_HPP
#define ZENI_CONCURRENCY_MUTEX_HPP

#include "Internal/Linkage.hpp"

#ifndef DISABLE_MULTITHREADING
#include <mutex>

namespace std {
  template class ZENI_CONCURRENCY_LINKAGE std::lock_guard<std::mutex>;
  class ZENI_CONCURRENCY_LINKAGE std::mutex;
}
#endif

namespace Zeni::Concurrency {

  class Mutex_Pimpl;
  class Mutex_Lock_Pimpl;

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
#ifndef DISABLE_MULTITHREADING
      std::lock_guard<std::mutex> m_lock;
#endif
    };

    Mutex() noexcept = default;

  private:
#ifndef DISABLE_MULTITHREADING
    std::mutex m_mutex;
#endif
  };

}

#endif
