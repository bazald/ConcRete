#include "Zeni/Concurrency/Internal/Reclamation_Stacks_Impl.hpp"

namespace Zeni::Concurrency {

  Reclamation_Stacks_Impl::Reclamation_Stacks_Impl() noexcept(false)
  {
  }

  Reclamation_Stacks_Impl & Reclamation_Stacks_Impl::get() noexcept(false) {
    static Reclamation_Stacks_Impl reclamation_stacks;
    return reclamation_stacks;
  }

  Reclamation_Stack * Reclamation_Stacks_Impl::get_stack() noexcept(false) {
    if (!m_reclamation_stack_inserted) {
      Mutex::Lock mutex_lock(m_mutex);
      m_reclamation_stacks.insert(&m_reclamation_stack);
    }

    return &m_reclamation_stack;
  }

  void Reclamation_Stacks_Impl::clear_stack() noexcept {
    if (m_reclamation_stack_inserted) {
      Mutex::Lock mutex_lock(m_mutex);
      m_reclamation_stacks.erase(&m_reclamation_stack);
    }
  }

  void Reclamation_Stacks_Impl::reclaim() noexcept {
    Mutex::Lock mutex_lock(m_mutex);
    for (auto &reclamation_stack : m_reclamation_stacks)
      reclamation_stack->reclaim();
  }

  thread_local Reclamation_Stack Reclamation_Stacks_Impl::m_reclamation_stack;
  thread_local bool Reclamation_Stacks_Impl::m_reclamation_stack_inserted;

}
