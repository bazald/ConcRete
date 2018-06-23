#ifndef ZENI_CONCURRENCY_QUEUE_HPP
#define ZENI_CONCURRENCY_QUEUE_HPP

#include "Reclamation_Stacks.hpp"

namespace Zeni::Concurrency {

  template <typename TYPE>
  class Queue {
    Queue(const Queue &) = delete;
    Queue & operator=(const Queue &) = delete;

    struct Node : public Reclamation_Stack::Node {
      Node() = default;

      std::atomic<Node *> next = nullptr;
      std::atomic<TYPE *> value_ptr = nullptr;
    };

  public:
    Queue() = default;

    ~Queue() noexcept {
      Node * head = m_head.load(std::memory_order_acquire);
      Node * tail = m_tail.load(std::memory_order_acquire);
      while (head != tail) {
        Node * next = head->next.load(std::memory_order_acquire);
        TYPE * value_ptr = head->value_ptr.load(std::memory_order_relaxed);
        delete head;
        delete value_ptr;
        head = next;
      }
      delete head;
    }

    bool empty() const {
      return m_head.load(std::memory_order_acquire) == m_tail.load(std::memory_order_acquire);
    }

    //int64_t size() const {
    //  return m_size.load(std::memory_order_relaxed);
    //}

    void push(const TYPE &value) {
      //m_size.fetch_add(1, std::memory_order_relaxed);
      m_writers.fetch_add(1, std::memory_order_release);
      Node * prev = m_tail.load(std::memory_order_relaxed);
      TYPE * value_ptr = new TYPE(value);
      Node * tail = new Node;
      for (;;) {
        TYPE * empty = nullptr;
        if (prev->value_ptr.compare_exchange_strong(empty, value_ptr, std::memory_order_release, std::memory_order_relaxed))
          break;
        else {
          Node * next = nullptr;
          if (prev->next.compare_exchange_strong(next, tail, std::memory_order_release, std::memory_order_acquire)) {
            m_tail.compare_exchange_strong(prev, tail, std::memory_order_release, std::memory_order_relaxed);
            tail = new Node;
          }
          else
            m_tail.compare_exchange_strong(prev, next, std::memory_order_release, std::memory_order_relaxed);
        }
        prev = m_tail.load(std::memory_order_relaxed);
      }
      Node * next = nullptr;
      if (prev->next.compare_exchange_strong(next, tail, std::memory_order_release, std::memory_order_relaxed))
        m_tail.compare_exchange_strong(prev, tail, std::memory_order_release, std::memory_order_relaxed);
      else {
        delete tail;
        m_tail.compare_exchange_strong(prev, next, std::memory_order_release, std::memory_order_relaxed);
      }
      m_writers.fetch_sub(1, std::memory_order_release);
    }

    void push(TYPE &&value) {
      //m_size.fetch_add(1, std::memory_order_relaxed);
      m_writers.fetch_add(1, std::memory_order_release);
      Node * prev = m_tail.load(std::memory_order_relaxed);
      TYPE * value_ptr = new TYPE(std::move(value));
      Node * tail = new Node;
      for (;;) {
        TYPE * empty = nullptr;
        if (prev->value_ptr.compare_exchange_strong(empty, value_ptr, std::memory_order_release, std::memory_order_relaxed))
          break;
        else {
          Node * next = nullptr;
          if (prev->next.compare_exchange_strong(next, tail, std::memory_order_release, std::memory_order_acquire)) {
            m_tail.compare_exchange_strong(prev, tail, std::memory_order_release, std::memory_order_relaxed);
            tail = new Node;
          }
          else
            m_tail.compare_exchange_strong(prev, next, std::memory_order_release, std::memory_order_relaxed);
        }
        prev = m_tail.load(std::memory_order_relaxed);
      }
      Node * next = nullptr;
      if (prev->next.compare_exchange_strong(next, tail, std::memory_order_release, std::memory_order_relaxed))
        m_tail.compare_exchange_strong(prev, tail, std::memory_order_release, std::memory_order_relaxed);
      else {
        delete tail;
        m_tail.compare_exchange_strong(prev, next, std::memory_order_release, std::memory_order_relaxed);
      }
      m_writers.fetch_sub(1, std::memory_order_release);
    }

    bool try_pop(TYPE &value) {
      m_writers.fetch_add(1, std::memory_order_release);
      Node * head = m_head.load(std::memory_order_acquire);
      Node * tail = m_tail.load(std::memory_order_acquire);
      for (;;) {
        if (head == tail) {
          m_writers.fetch_sub(1, std::memory_order_release);
          return false;
        }
        Node * next = head->next.load(std::memory_order_relaxed);
        if (m_head.compare_exchange_strong(head, next, std::memory_order_release, std::memory_order_relaxed))
          break;
        else
          tail = m_tail.load(std::memory_order_relaxed);
      }
      TYPE * value_ptr = head->value_ptr.load(std::memory_order_relaxed);
      if (m_writers.fetch_sub(1, std::memory_order_acq_rel) == 1)
        delete head;
      else
        Reclamation_Stacks::push(head);
      value = std::move(*value_ptr);
      delete value_ptr;
      //m_size.fetch_sub(1, std::memory_order_relaxed);
      return true;
    }

  private:
    std::atomic<Node *> m_head = new Node();
    std::atomic<Node *> m_tail = m_head.load(std::memory_order_relaxed);
    //std::atomic_int64_t m_size = 0;
    std::atomic<int64_t> m_writers = 0;
  };

}

#endif
