#ifndef ZENI_CONCURRENCY_STACK_HPP
#define ZENI_CONCURRENCY_STACK_HPP

#include "Reclamation_Stacks.hpp"

namespace Zeni::Concurrency {

  template <typename TYPE>
  class Stack {
    Stack(const Stack &) = delete;
    Stack & operator=(const Stack &) = delete;

    struct Node : public Reclamation_Stack::Node {
      Node() = default;
      Node(Node * const &next_, const TYPE &value_) : Reclamation_Stack::Node(next_), value(value_) {}
      Node(Node * const &next_, TYPE &&value_) : Reclamation_Stack::Node(next_), value(std::move(value_)) {}
      Node(Node * &&next_, const TYPE &value_) : Reclamation_Stack::Node(std::move(next_)), value(value_) {}
      Node(Node * &&next_, TYPE &&value_) : Reclamation_Stack::Node(std::move(next_)), value(std::move(value_)) {}

      TYPE value;
    };

  public:
    Stack() = default;

    bool empty() const {
      return m_head.load(std::memory_order_acquire) == nullptr;
    }

    //int64_t size() const {
    //  return m_size.load(std::memory_order_relaxed);
    //}

    void push(const TYPE &value) {
      //m_size.fetch_add(1, std::memory_order_relaxed);
      Node * const head = new Node(m_head.load(std::memory_order_relaxed), value);
      while (!m_head.compare_exchange_weak(reinterpret_cast<Node *&>(head->next), head, std::memory_order_release, std::memory_order_relaxed));
    }

    bool try_pop(TYPE &value) {
      m_poppers.fetch_add(1, std::memory_order_release);
      Node * head;
      do {
        head = m_head.load(std::memory_order_relaxed);
        if (!head) {
          m_poppers.fetch_sub(1, std::memory_order_release);
          return false;
        }
      } while (!m_head.compare_exchange_weak(head, static_cast<Node *>(head->next), std::memory_order_release, std::memory_order_relaxed));
      //m_size.fetch_sub(1, std::memory_order_relaxed);
      value = head->value;
      if (m_poppers.fetch_sub(1, std::memory_order_acq_rel) == 1)
        delete head;
      else
        Reclamation_Stacks::push(head);
      return true;
    }

  private:
    std::atomic<Node *> m_head;
    //std::atomic_int64_t m_size = 0;
    std::atomic<int64_t> m_poppers = 0;
  };

}

#endif
