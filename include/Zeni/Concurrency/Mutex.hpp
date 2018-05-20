#ifndef ZENI_CONCURRENCY_MUTEX_H
#define ZENI_CONCURRENCY_MUTEX_H

#include "Zeni/Concurrency/Linkage.hpp"

namespace Zeni {

  namespace Concurrency {

    class Mutex_Pimpl;
    class Mutex_Lock_Pimpl;

    class ZENI_CONCURRENCY_LINKAGE Mutex {
      Mutex(const Mutex &) = delete;
      Mutex & operator=(const Mutex &) = delete;

      friend class Mutex_Lock_Pimpl;

    public:
      class ZENI_CONCURRENCY_LINKAGE Lock {
        Lock(const Lock &) = delete;
        Lock & operator=(const Lock &) = delete;

      public:
        Lock(Mutex &mutex);
        ~Lock();

      private:
        Mutex_Lock_Pimpl * const m_impl;
      };


      Mutex();
      ~Mutex();

    private:
      Mutex_Pimpl * const m_impl;
    };

  }

}

#endif
