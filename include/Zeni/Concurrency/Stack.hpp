#ifndef ZENI_CONCURRENCY_STACK_HPP
#define ZENI_CONCURRENCY_STACK_HPP

#include "Internal/Linkage.hpp"

#include <atomic>
#include <memory>

namespace Zeni::Concurrency {

  template <typename TYPE>
  class Stack {
    Stack(const Stack &) = delete;
    Stack & operator=(const Stack &) = delete;

    struct Node {
      Node() = default;
      Node(const TYPE &value_, const std::shared_ptr<Node> &next_) : value(value_), next(next_) {}
      Node(TYPE &&value_, const std::shared_ptr<Node> &next_) : value(std::move(value_)), next(next_) {}

      TYPE value;
      std::shared_ptr<Node> next;
    };

  public:
    Stack() = default;

    bool empty() const {
      return std::atomic_load_explicit(&m_head, std::memory_order_relaxed) == nullptr;
    }

    //int64_t size() const {
    //  return m_size.load(std::memory_order_relaxed);
    //}

    void push(const TYPE &value) {
      //m_size.fetch_add(1, std::memory_order_relaxed);
      auto head = std::make_shared<Node>(value, std::atomic_load_explicit(&m_head, std::memory_order_relaxed));
      while (!std::atomic_compare_exchange_weak_explicit(&m_head, &head->next, head, std::memory_order_release, std::memory_order_relaxed))
        head->next = std::atomic_load_explicit(&m_head, std::memory_order_relaxed);
    }

    bool try_pop(TYPE &value) {
      std::shared_ptr<Node> head;
      do {
        head = std::atomic_load_explicit(&m_head, std::memory_order_relaxed);
        if (!head)
          return false;
      } while (!std::atomic_compare_exchange_weak_explicit(&m_head, &head, head->next, std::memory_order_release, std::memory_order_relaxed));
      //m_size.fetch_sub(1, std::memory_order_relaxed);
      value = head->value;
      return true;
    }

  private:
    std::shared_ptr<Node> m_head;
    //std::atomic_int64_t m_size = 0;
  };

}

#endif
