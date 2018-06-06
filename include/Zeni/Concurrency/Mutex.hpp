#ifndef ZENI_CONCURRENCY_MUTEX_HPP
#define ZENI_CONCURRENCY_MUTEX_HPP

#include "Internal/Linkage.hpp"

namespace Zeni::Concurrency {

  class Mutex_Pimpl;
  class Mutex_Lock_Pimpl;

  class ZENI_CONCURRENCY_LINKAGE Mutex {
    Mutex(const Mutex &) = delete;
    Mutex & operator=(const Mutex &) = delete;

#if defined(_MSC_VER)
    static const int m_pimpl_size = 80;
#else
    static const int m_pimpl_size = 40;
#endif
    static const int m_pimpl_align = 8;
    const Mutex_Pimpl * get_pimpl() const noexcept;
    Mutex_Pimpl * get_pimpl() noexcept;

    friend class Mutex_Lock_Pimpl;

  public:
    class ZENI_CONCURRENCY_LINKAGE Lock {
      Lock(const Lock &) = delete;
      Lock & operator=(const Lock &) = delete;

      static const int m_pimpl_size = 8;
      static const int m_pimpl_align = 8;
      const Mutex_Lock_Pimpl * get_pimpl() const noexcept;
      Mutex_Lock_Pimpl * get_pimpl() noexcept;

    public:
      Lock(Mutex &mutex) noexcept;
      ~Lock() noexcept;

    private:
      alignas(m_pimpl_align) char m_pimpl_storage[m_pimpl_size];
    };

    Mutex() noexcept;
    ~Mutex() noexcept;

  private:
    alignas(m_pimpl_align) char m_pimpl_storage[m_pimpl_size];
  };

}

#endif
