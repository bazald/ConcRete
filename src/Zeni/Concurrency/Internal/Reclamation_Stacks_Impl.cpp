#include "Zeni/Concurrency/Internal/Reclamation_Stacks_Impl.hpp"

namespace Zeni::Concurrency {

  Reclamation_Stacks_Impl::Clearer::~Clearer() {
    Reclamation_Stacks_Impl::get().clear_stack();
  }

  Reclamation_Stacks_Impl::Reclamation_Stacks_Impl() noexcept(false)
  {
  }

  Reclamation_Stacks_Impl & Reclamation_Stacks_Impl::get() noexcept(false) {
    static Reclamation_Stacks_Impl reclamation_stacks;
    return reclamation_stacks;
  }

  std::shared_ptr<Reclamation_Stack> Reclamation_Stacks_Impl::get_stack() noexcept(false) {
    if (!m_reclamation_stack) {
      m_reclamation_stack = std::allocate_shared<Reclamation_Stack>(Mallocator<Reclamation_Stack>());
      m_clearer = std::allocate_shared<Clearer>(Mallocator<Clearer>());

      {
        Mutex::Lock mutex_lock(m_mutex);
        m_reclamation_stacks.emplace(m_reclamation_stack);
      }
    }

    return m_reclamation_stack;
  }

  void Reclamation_Stacks_Impl::clear_stack() noexcept {
    if (m_reclamation_stack) {
      Mutex::Lock mutex_lock(m_mutex);
      m_reclamation_stacks.erase(m_reclamation_stack);
    }
    m_reclamation_stack.reset();
  }

  void Reclamation_Stacks_Impl::reclaim() noexcept {
    Mutex::Lock mutex_lock(m_mutex);
    for (auto &reclamation_stack : m_reclamation_stacks)
      reclamation_stack->reclaim();
  }

  thread_local std::shared_ptr<Reclamation_Stack> Reclamation_Stacks_Impl::m_reclamation_stack;
  thread_local std::shared_ptr<Reclamation_Stacks_Impl::Clearer> Reclamation_Stacks_Impl::m_clearer;

}
