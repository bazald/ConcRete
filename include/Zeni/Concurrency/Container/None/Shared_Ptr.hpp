#ifndef ZENI_CONCURRENCY_SHARED_PTR_HPP
#define ZENI_CONCURRENCY_SHARED_PTR_HPP

#include <memory>

namespace Zeni::Concurrency {

  template <typename TYPE>
  class Shared_Ptr : public std::shared_ptr<TYPE> {
  public:
    typedef Shared_Ptr<TYPE> Lock;

    Shared_Ptr() = default;

    Shared_Ptr(TYPE * const ptr) : std::shared_ptr<TYPE>(ptr) {}

    Shared_Ptr load() const {
      return *this;
    }
  };

}

#endif
