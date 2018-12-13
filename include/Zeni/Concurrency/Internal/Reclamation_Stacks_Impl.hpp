#ifndef ZENI_CONCURRENCY_RECLAMATION_STACKS_IMPL_HPP
#define ZENI_CONCURRENCY_RECLAMATION_STACKS_IMPL_HPP

#include "Reclamation_Stack.hpp"
#include "../Mutex.hpp"

#include <cstdint>
#include <list>
#include <unordered_set>

namespace Zeni::Concurrency {

  class Reclamation_Stacks_Impl {
    Reclamation_Stacks_Impl(const Reclamation_Stacks_Impl &) = delete;
    Reclamation_Stacks_Impl operator=(const Reclamation_Stacks_Impl &) = delete;

    Reclamation_Stacks_Impl() noexcept(false);

  public:
    static Reclamation_Stacks_Impl & get() noexcept(false);

    Reclamation_Stack * get_stack() noexcept(false);

    void clear_stack() noexcept;

    void reclaim() noexcept;

  private:
    Mutex m_mutex;
    std::unordered_set<Reclamation_Stack *> m_reclamation_stacks;
    static thread_local Reclamation_Stack m_reclamation_stack;
    static thread_local bool m_reclamation_stack_inserted;
  };

}

#endif
