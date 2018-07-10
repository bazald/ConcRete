#ifndef ZENI_CONCURRENCY_QUEUE_HPP
#define ZENI_CONCURRENCY_QUEUE_HPP

#include "../../Internal/Reclamation_Stacks.hpp"
#include "Shared_Ptr.hpp"

namespace Zeni::Concurrency {

  template <typename TYPE, bool POP_ASSISTS_PUSH = true>
  class Queue {
    Queue(const Queue &) = delete;
    Queue & operator=(const Queue &) = delete;

    struct Node : public Reclamation_Stack::Node {
      std::atomic<Node *> next = nullptr;
      Shared_Ptr<TYPE> value_ptr = nullptr;
    };

  public:
    Queue() noexcept {
      std::atomic_thread_fence(std::memory_order_release);
    }

    ~Queue() noexcept {
      std::atomic_thread_fence(std::memory_order_acquire);
      Node * head = m_head.load(std::memory_order_relaxed);
      while (head) {
        Node * const next = head->next.load(std::memory_order_relaxed);
        delete head;
        head = next;
      }
    }

    bool empty() const {
      return m_head.load(std::memory_order_relaxed) == m_tail.load(std::memory_order_relaxed);
    }

    //int64_t size() const {
    //  return m_size.load(std::memory_order_relaxed);
    //}

    //int64_t size() const {
    //  int64_t sz = 0;
    //  Node * head = m_head.load(std::memory_order_relaxed);
    //  for (;;) {
    //    Node * const next = head->next.load(std::memory_order_relaxed);
    //    if (!next || !head->value_ptr.load(std::memory_order_relaxed))
    //      break;
    //    ++sz;
    //    head = next;
    //  }
    //  return sz;
    //}

    void push(const TYPE &value) {
      push(new TYPE(value));
    }

    void push(TYPE &&value) {
      push(new TYPE(std::move(value)));
    }

    bool try_pop(TYPE &value) {
      m_writers.fetch_add(1, std::memory_order_relaxed);
      Node * head = m_head.load(std::memory_order_relaxed);
      typename Shared_Ptr<TYPE>::Lock value_ptr;
      if (POP_ASSISTS_PUSH) {
        Node * new_tail = nullptr;
        for (;;) {
          value_ptr = head->value_ptr.load();
          if (!value_ptr) {
            m_writers.fetch_sub(1, std::memory_order_relaxed);
            return false;
          }
          Node * next = head->next.load(std::memory_order_relaxed);
          if (!next) {
            if (!new_tail) {
              new_tail = new Node;
              std::atomic_thread_fence(std::memory_order_release);
            }
            next = new_tail;
            if (push_pointers_pop(head, next))
              new_tail = nullptr;
            else {
              delete new_tail;
              new_tail = nullptr;
              head = m_head.load(std::memory_order_relaxed);
              continue;
            }
          }
          if (m_head.compare_exchange_strong(head, next, std::memory_order_relaxed, std::memory_order_relaxed))
            break;
        }
        delete new_tail;
      }
      else {
        for (;;) {
          Node * next = head->next.load(std::memory_order_relaxed);
          if (!next) {
            m_writers.fetch_sub(1, std::memory_order_relaxed);
            return false;
          }
          if (m_head.compare_exchange_strong(head, next, std::memory_order_relaxed, std::memory_order_relaxed))
            break;
        }
        value_ptr = head->value_ptr.load();
      }
      if (m_writers.fetch_sub(1, std::memory_order_relaxed) == 1)
        delete head;
      else
        Reclamation_Stacks::push(head);
      std::atomic_thread_fence(std::memory_order_acquire);
      value = std::move(*value_ptr);
      //m_size.fetch_sub(1, std::memory_order_relaxed);
      return true;
    }

  private:
    void push(const typename Shared_Ptr<TYPE>::Lock value_ptr) {
      //m_size.fetch_add(1, std::memory_order_relaxed);
      m_writers.fetch_add(1, std::memory_order_relaxed);
      Node * new_tail = new Node;
      std::atomic_thread_fence(std::memory_order_release);
      Node * old_tail = m_tail.load(std::memory_order_relaxed);
      for (;;) {
        typename Shared_Ptr<TYPE>::Lock empty = nullptr;
        if (old_tail->value_ptr.compare_exchange_strong(empty, value_ptr)) {
          if (!push_pointers_push(old_tail, new_tail))
            delete new_tail;
          m_writers.fetch_sub(1, std::memory_order_relaxed);
          return;
        }
        else {
          if (push_pointers_push(old_tail, new_tail)) {
            new_tail = new Node;
            std::atomic_thread_fence(std::memory_order_release);
          }
        }
      }
    }

    // Return true if new tail pointer is used, otherwise false
    bool push_pointers_pop(Node * old_tail, Node * &new_tail) {
      Node * next = nullptr;
      if (old_tail->next.compare_exchange_strong(next, new_tail, std::memory_order_relaxed, std::memory_order_relaxed)) {
        m_tail.compare_exchange_strong(old_tail, new_tail, std::memory_order_relaxed, std::memory_order_relaxed);
        return true;
      }
      else {
        m_tail.compare_exchange_strong(old_tail, next, std::memory_order_relaxed, std::memory_order_relaxed);
        return false;
      }
    }

    // Return true if new tail pointer is used, otherwise false
    bool push_pointers_push(Node * &old_tail, Node * new_tail) {
      Node * next = nullptr;
      if (old_tail->next.compare_exchange_strong(next, new_tail, std::memory_order_relaxed, std::memory_order_relaxed)) {
        m_tail.compare_exchange_strong(old_tail, new_tail, std::memory_order_relaxed, std::memory_order_relaxed);
        return true;
      }
      else {
        m_tail.compare_exchange_strong(old_tail, next, std::memory_order_relaxed, std::memory_order_relaxed);
        return false;
      }
    }

    std::atomic<Node *> m_head = new Node();
    std::atomic<Node *> m_tail = m_head.load(std::memory_order_relaxed);
    //std::atomic_int64_t m_size = 0;
    std::atomic_int64_t m_writers = 0;
  };

}

#endif
