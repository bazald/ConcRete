#include "Zeni/Concurrency/Internal/Reclamation_Stack.hpp"

namespace Zeni::Concurrency {

  Reclamation_Stack::~Reclamation_Stack() noexcept {
    while (m_head3) {
      const Node * const head = m_head3;
      m_head3 = head->reclamation_next;
      delete head;
    }

    while (m_head2) {
      const Node * const head = m_head2;
      m_head2 = head->reclamation_next;
      delete head;
    }

    while (m_head) {
      const Node * const head = m_head;
      m_head = head->reclamation_next;
      delete head;
    }
  }

  Reclamation_Stack & Reclamation_Stack::get() noexcept {
    static thread_local Reclamation_Stack g_reclamation_stack;
    return g_reclamation_stack;
  }

  void Reclamation_Stack::push(const Reclamation_Stack::Node * const node) noexcept {
    //m_size.fetch_add(1, std::memory_order_relaxed);

    auto &reclamation_stack = get();

    node->reclamation_next = reclamation_stack.m_head;
    reclamation_stack.m_head = node;
  }

  void Reclamation_Stack::reclaim() noexcept {
    auto &reclamation_stack = get();

    while (reclamation_stack.m_head3) {
      const Node * const head = reclamation_stack.m_head3;
      reclamation_stack.m_head3 = head->reclamation_next;
      delete head;
    }

    reclamation_stack.m_head3 = reclamation_stack.m_head2;
    reclamation_stack.m_head2 = reclamation_stack.m_head;
    reclamation_stack.m_head = nullptr;
  }

}
