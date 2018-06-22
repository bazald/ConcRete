#include "Zeni/Concurrency/Internal/Reclamation_Stacks_Impl.hpp"
#include "Zeni/Concurrency/Reclamation_Stacks.hpp"

namespace Zeni::Concurrency {

  std::shared_ptr<Reclamation_Stack> Reclamation_Stacks::get_stack() noexcept(false) {
    return Reclamation_Stacks_Impl::get().get_stack();
  }

  void Reclamation_Stacks::push(Reclamation_Stack::Node * const node) noexcept {
    Reclamation_Stacks_Impl::get().get_stack()->push(node);
  }

  void Reclamation_Stacks::reclaim() noexcept {
    Reclamation_Stacks_Impl::get().reclaim();
  }

}
