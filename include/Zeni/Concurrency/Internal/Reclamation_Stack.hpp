#ifndef ZENI_CONCURRENCY_RECLAMATION_STACK_HPP
#define ZENI_CONCURRENCY_RECLAMATION_STACK_HPP

#include <atomic>
#include <cassert>
#include <cstdint>

namespace Zeni::Concurrency {

  class Reclamation_Stack {
    Reclamation_Stack(const Reclamation_Stack &) = delete;
    Reclamation_Stack & operator=(const Reclamation_Stack &) = delete;

  public:
    struct Node {
      virtual ~Node() = default;

      mutable const Node * reclamation_next = nullptr;
    };

    Reclamation_Stack() = default;

    ~Reclamation_Stack() noexcept {
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

    bool empty() const noexcept {
      return !m_head;
    }

    //int64_t size() const {
    //  return m_size.load(std::memory_order_relaxed);
    //}

    void push(const Node * const node) noexcept {
      //m_size.fetch_add(1, std::memory_order_relaxed);
      node->reclamation_next = m_head;
      m_head = node;
    }

    void reclaim() noexcept {
      while (m_head3) {
        const Node * const head = m_head3;
        m_head3 = head->reclamation_next;
        delete head;
      }

      m_head3 = m_head2;
      m_head2 = m_head;
      m_head = nullptr;
    }

  private:
    const Node * m_head = nullptr;
    const Node * m_head2 = nullptr;
    const Node * m_head3 = nullptr;
    //ZENI_CONCURRENCY_CACHE_ALIGN std::atomic_int64_t m_size = 0;
  };

}

#endif
