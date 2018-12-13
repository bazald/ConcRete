#ifndef ZENI_CONCURRENCY_RECLAMATION_STACKS_HPP
#define ZENI_CONCURRENCY_RECLAMATION_STACKS_HPP

#include "Linkage.hpp"
#include "Reclamation_Stack.hpp"

namespace Zeni::Concurrency {

  class Reclamation_Stack;

  class Reclamation_Stacks {
    Reclamation_Stacks(const Reclamation_Stacks &) = delete;
    Reclamation_Stacks & operator=(const Reclamation_Stacks &) = delete;

  public:
    ZENI_CONCURRENCY_LINKAGE static Reclamation_Stack * get_stack() noexcept(false);

    ZENI_CONCURRENCY_LINKAGE static void push(const Reclamation_Stack::Node * const node) noexcept;

    ZENI_CONCURRENCY_LINKAGE static void reclaim() noexcept;
  };

}

#endif
