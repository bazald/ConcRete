#ifndef ZENI_CONCURRENCY_MUTEX_HPP
#define ZENI_CONCURRENCY_MUTEX_HPP

#include "Linkage.hpp"

namespace Zeni::Concurrency {

  class Mutex_Pimpl;
  class Mutex_Lock_Pimpl;

  class Mutex {
    Mutex(const Mutex &) = delete;
    Mutex & operator=(const Mutex &) = delete;

    static const int m_pimpl_size = 80;
    static const int m_pimpl_align = 8;
    const Mutex_Pimpl * get_pimpl() const noexcept;
    Mutex_Pimpl * get_pimpl() noexcept;

    friend class Mutex_Lock_Pimpl;

  public:
    class Lock {
      Lock(const Lock &) = delete;
      Lock & operator=(const Lock &) = delete;

      static const int m_pimpl_size = 8;
      static const int m_pimpl_align = 8;
      const Mutex_Lock_Pimpl * get_pimpl() const noexcept;
      Mutex_Lock_Pimpl * get_pimpl() noexcept;

    public:
      ZENI_CONCURRENCY_LINKAGE Lock(Mutex &mutex) noexcept;
      ZENI_CONCURRENCY_LINKAGE ~Lock() noexcept;

    private:
      alignas(m_pimpl_align) char m_pimpl_storage[m_pimpl_size];
    };

    ZENI_CONCURRENCY_LINKAGE Mutex() noexcept;
    ZENI_CONCURRENCY_LINKAGE ~Mutex() noexcept;

  private:
    alignas(m_pimpl_align) char m_pimpl_storage[m_pimpl_size];
  };

}

#endif
