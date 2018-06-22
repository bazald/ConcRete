#ifndef ZENI_CONCURRENCY_RECLAMATION_STACK_HPP
#define ZENI_CONCURRENCY_RECLAMATION_STACK_HPP

#include <atomic>
#include <cassert>
#include <cstdint>
#include <memory>

namespace Zeni::Concurrency {

  class Reclamation_Stack : public std::enable_shared_from_this<Reclamation_Stack> {
    Reclamation_Stack(const Reclamation_Stack &) = delete;
    Reclamation_Stack & operator=(const Reclamation_Stack &) = delete;

  public:
    struct Node {
      Node * reclaim_next = nullptr;
    };

    Reclamation_Stack() = default;

    ~Reclamation_Stack() noexcept {
      reclaim();
    }

    bool empty() const noexcept {
      return !m_head;
    }

    //int64_t size() const {
    //  return m_size.load(std::memory_order_relaxed);
    //}

    void push(Node * const node) noexcept {
      //m_size.fetch_add(1, std::memory_order_relaxed);
      assert(node->reclaim_next == nullptr);
      node->reclaim_next = m_head;
      m_head = node;
    }

    void reclaim() noexcept {
      while (m_head) {
        Node * const reclaim_next = m_head->reclaim_next;
        delete m_head;
        m_head = reclaim_next;
      }
    }

  private:
    Node * m_head = nullptr;
    //std::atomic_int64_t m_size = 0;
  };

}

#endif