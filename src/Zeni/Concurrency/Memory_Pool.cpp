#include "Zeni/Concurrency/Internal/Memory_Pool_Impl.hpp"

namespace Zeni::Concurrency {

  std::shared_ptr<Memory_Pool> Memory_Pool::Create() noexcept(false) {
    return std::make_shared<Memory_Pool_Impl>();
  }

}
