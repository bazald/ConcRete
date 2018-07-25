#ifndef ZENI_CONCURRENCY_INTRUSIVE_SHARED_PTR_HPP
#define ZENI_CONCURRENCY_INTRUSIVE_SHARED_PTR_HPP

#include <memory>

namespace Zeni::Concurrency {

  template <typename TYPE>
  class Intrusive_Shared_Ptr : public std::shared_ptr<TYPE> {
  public:
    typedef Intrusive_Shared_Ptr<TYPE> Lock;

    Intrusive_Shared_Ptr() = default;

    Intrusive_Shared_Ptr(TYPE * const ptr) : std::shared_ptr<TYPE>(ptr) {}

    Intrusive_Shared_Ptr load() const {
      return *this;
    }
  };

  template <typename TYPE>
  class Enable_Intrusive_Sharing : public std::enable_shared_from_this<TYPE> {};

}

#endif
