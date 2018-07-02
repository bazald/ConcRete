#ifndef ZENI_CONCURRENCY_STACK_HPP
#define ZENI_CONCURRENCY_STACK_HPP

#include "../Internal/Reclamation_Stacks.hpp"

namespace Zeni::Concurrency {

  template <typename TYPE>
  class Stack {
    Stack(const Stack &) = delete;
    Stack & operator=(const Stack &) = delete;

    struct Node : public Reclamation_Stack::Node {
      Node() = default;
      Node(Node * const &next_, const TYPE &value_) : next(next_), value(value_) {}
      Node(Node * const &next_, TYPE &&value_) : next(next_), value(std::move(value_)) {}
      Node(Node * &&next_, const TYPE &value_) : next(std::move(next_)), value(value_) {}
      Node(Node * &&next_, TYPE &&value_) : next(std::move(next_)), value(std::move(value_)) {}

      Node * next = nullptr;
      TYPE value;
    };

  public:
    Stack() noexcept {
      std::atomic_thread_fence(std::memory_order_release);
    }

    ~Stack() noexcept {
      std::atomic_thread_fence(std::memory_order_acquire);
      Node * head = m_head.load(std::memory_order_relaxed);
      while (head) {
        Node * next = head->next;
        delete head;
        head = next;
      }
    }

    bool empty() const {
      return m_head.load(std::memory_order_relaxed) == nullptr;
    }

    //int64_t size() const {
    //  return m_size.load(std::memory_order_relaxed);
    //}

    void push(const TYPE &value) {
      //m_size.fetch_add(1, std::memory_order_relaxed);
      Node * const head = new Node(m_head.load(std::memory_order_relaxed), value);
      std::atomic_thread_fence(std::memory_order_release);
      while (!m_head.compare_exchange_weak(head->next, head, std::memory_order_relaxed, std::memory_order_relaxed));
    }

    void push(TYPE &&value) {
      //m_size.fetch_add(1, std::memory_order_relaxed);
      Node * const head = new Node(m_head.load(std::memory_order_relaxed), std::move(value));
      std::atomic_thread_fence(std::memory_order_release);
      while (!m_head.compare_exchange_weak(head->next, head, std::memory_order_relaxed, std::memory_order_relaxed));
    }

    bool try_pop(TYPE &value) {
      m_poppers.fetch_add(1, std::memory_order_relaxed);
      Node * head = m_head.load(std::memory_order_relaxed);
      do {
        if (!head) {
          m_poppers.fetch_sub(1, std::memory_order_relaxed);
          return false;
        }
      } while (!m_head.compare_exchange_weak(head, head->next, std::memory_order_relaxed, std::memory_order_relaxed));
      std::atomic_thread_fence(std::memory_order_acquire);
      value = std::move(head->value);
      if (m_poppers.fetch_sub(1, std::memory_order_relaxed) == 1)
        delete head;
      else
        Reclamation_Stacks::push(head);
      //m_size.fetch_sub(1, std::memory_order_relaxed);
      return true;
    }

  private:
    std::atomic<Node *> m_head = nullptr;
    //std::atomic_int64_t m_size = 0;
    std::atomic_int64_t m_poppers = 0;
  };

}

#endif
