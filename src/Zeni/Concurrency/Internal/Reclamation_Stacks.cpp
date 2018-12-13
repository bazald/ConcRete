#include "Zeni/Concurrency/Internal/Reclamation_Stacks.hpp"

#include "Zeni/Concurrency/Internal/Reclamation_Stacks_Impl.hpp"

namespace Zeni::Concurrency {

  Reclamation_Stack * Reclamation_Stacks::get_stack() noexcept(false) {
    return Reclamation_Stacks_Impl::get().get_stack();
  }

  void Reclamation_Stacks::push(const Reclamation_Stack::Node * const node) noexcept {
    Reclamation_Stacks_Impl::get().get_stack()->push(node);
  }

  void Reclamation_Stacks::reclaim() noexcept {
    Reclamation_Stacks_Impl::get().reclaim();
  }

}
