#include "Zeni/Concurrency/Internal/Reclamation_Stack.hpp"

namespace Zeni::Concurrency {

  Reclamation_Stack::Reclamation_Stack() noexcept
  {
  }

  Reclamation_Stack::~Reclamation_Stack() noexcept {
    assert(m_destroyed);
    assert(!m_head);
    assert(!m_head2);
    assert(!m_head3);
  }

  Reclamation_Stack & Reclamation_Stack::get() noexcept {
    static thread_local Reclamation_Stack g_reclamation_stack;

    static thread_local struct Cleaner {
      ~Cleaner() {
        final_reclaim();
      }
    } g_cleaner;

    return g_reclamation_stack;
  }

  void Reclamation_Stack::push(const Reclamation_Stack::Node * const node) noexcept {
    //m_size.fetch_add(1, std::memory_order_relaxed);

    auto &reclamation_stack = get();

    assert(!reclamation_stack.m_destroyed);

    ++reclamation_stack.m_size;

    node->reclamation_next = reclamation_stack.m_head;
    reclamation_stack.m_head = node;
  }

  void Reclamation_Stack::reclaim() noexcept {
    auto &reclamation_stack = get();

    assert(!reclamation_stack.m_destroyed);

    if (reclamation_stack.m_middestruction)
      return;

    while (reclamation_stack.m_head3) {
      const Node * const head = reclamation_stack.m_head3;
      reclamation_stack.m_head3 = head->reclamation_next;
      delete head;

      --reclamation_stack.m_size;
    }

    reclamation_stack.m_head3 = reclamation_stack.m_head2;
    reclamation_stack.m_head2 = reclamation_stack.m_head;
    reclamation_stack.m_head = nullptr;
  }

  void Reclamation_Stack::final_reclaim() noexcept {
    auto &reclamation_stack = get();

    assert(!reclamation_stack.m_middestruction);
    assert(!reclamation_stack.m_destroyed);

    reclamation_stack.m_middestruction = true;

    while (reclamation_stack.m_head3) {
      const Node * const head = reclamation_stack.m_head3;
      reclamation_stack.m_head3 = head->reclamation_next;
      delete head;

      --reclamation_stack.m_size;
    }

    while (reclamation_stack.m_head2) {
      const Node * const head = reclamation_stack.m_head2;
      reclamation_stack.m_head2 = head->reclamation_next;
      delete head;

      --reclamation_stack.m_size;
    }

    while (reclamation_stack.m_head) {
      const Node * const head = reclamation_stack.m_head;
      reclamation_stack.m_head = head->reclamation_next;
      delete head;

      --reclamation_stack.m_size;
    }

    assert(!reclamation_stack.m_head);
    assert(!reclamation_stack.m_head2);
    assert(!reclamation_stack.m_head3);
    assert(!reclamation_stack.m_destroyed);

    reclamation_stack.m_destroyed = true;
    reclamation_stack.m_middestruction = false;
  }

}
