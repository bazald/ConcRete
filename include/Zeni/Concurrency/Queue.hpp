#ifndef ZENI_CONCURRENCY_QUEUE_HPP
#define ZENI_CONCURRENCY_QUEUE_HPP

#include "Internal/Linkage.hpp"

#include <atomic>
#include <memory>

namespace Zeni::Concurrency {

  template <typename TYPE>
  class Queue {
    Queue(const Queue &) = delete;
    Queue & operator=(const Queue &) = delete;

    struct Node {
      Node() = default;
      Node(const TYPE &value_, const std::shared_ptr<Node> &next_) : value(value_), next(next_) {}
      Node(TYPE &&value_, const std::shared_ptr<Node> &next_) : value(std::move(value_)), next(next_) {}

      TYPE value;
      std::shared_ptr<Node> next;
      std::shared_ptr<Node> prev;
    };

  public:
    Queue() = default;

    ~Queue() {
      while (m_head) {
        std::shared_ptr<Node> next = m_head->next;
        m_head->next = nullptr;
        m_head->prev = nullptr;
        m_head = next;
      }
      m_tail = nullptr;
    }

    bool empty() const {
      return std::atomic_load_explicit(&m_head, std::memory_order_relaxed) == nullptr;
    }

    //int64_t size() const {
    //  return m_size.load(std::memory_order_relaxed);
    //}

    void push(const TYPE &value) {
      //m_size.fetch_add(1, std::memory_order_relaxed);
      std::shared_ptr<Node> next = std::atomic_load_explicit(&m_head, std::memory_order_relaxed);
      std::shared_ptr<Node> head = std::make_shared<Node>(value, next);
      while (!std::atomic_compare_exchange_weak_explicit(&m_head, &next, head, std::memory_order_release, std::memory_order_relaxed))
        head->next = next;
      // Invariant currently violated: next->prev not yet set to head
      if (next) {
        std::atomic_store_explicit(&next->prev, head, std::memory_order_release);
        // Invariant corrected: next->prev set to head
        // Potential delay side-effects: m_tail could both be incorrectly set to nullptr
        //if (next->popped.load(std::memory_order_acquire)) {
        //  std::shared_ptr<Node> tail = std::atomic_load_explicit(&m_tail, std::memory_order_acquire);
        //  while ((!tail || tail == next) && !head->popped.load(std::memory_order_acquire)) {
        //    if (std::atomic_compare_exchange_weak_explicit(&m_tail, &tail, head, std::memory_order_release, std::memory_order_acquire))
        //      return;
        //  }
        //}
      }
      else
        std::atomic_store_explicit(&m_tail, head, std::memory_order_release);
    }

    bool try_pop(TYPE &value) {
      std::shared_ptr<Node> tail, prev;
      do {
        tail = std::atomic_load_explicit(&m_tail, std::memory_order_relaxed);
        if (!tail)
          return false;
        prev = std::atomic_load_explicit(&tail->prev, std::memory_order_relaxed);
      } while (!std::atomic_compare_exchange_weak_explicit(&m_tail, &tail, prev, std::memory_order_release, std::memory_order_relaxed));
      // Potential invariant violation: prev and m_tail may incorrectly be set to nullptr
      //   Ignore. Let push() fix it.
      if (prev)
        std::atomic_store_explicit(&prev->next, std::shared_ptr<Node>(), std::memory_order_release);
      else
        std::atomic_compare_exchange_strong_explicit(&m_head, &tail, std::shared_ptr<Node>(), std::memory_order_acq_rel, std::memory_order_relaxed);
      std::atomic_store_explicit(&tail->prev, std::shared_ptr<Node>(), std::memory_order_relaxed);
      std::atomic_store_explicit(&tail->next, std::shared_ptr<Node>(), std::memory_order_relaxed);
      //m_size.fetch_sub(1, std::memory_order_relaxed);
      value = tail->value;
      return true;
    }

    //bool try_pop(TYPE &value) {
    //  std::shared_ptr<Node> tail = std::atomic_load_explicit(&m_tail, std::memory_order_relaxed);
    //  std::shared_ptr<Node> prev;
    //  for (;;) {
    //    if (!tail)
    //      return false;
    //    prev = std::atomic_load_explicit(&tail->prev, std::memory_order_acquire);
    //    if (std::atomic_compare_exchange_weak_explicit(&m_tail, &tail, prev, std::memory_order_release, std::memory_order_relaxed))
    //      break;
    //  }
    //  std::shared_ptr<Node> tail_copy = tail;
    //  while (!prev && !std::atomic_compare_exchange_weak_explicit(&m_head, &tail_copy, std::shared_ptr<Node>(), std::memory_order_release, std::memory_order_relaxed))
    //    prev = std::atomic_load_explicit(&tail->prev, std::memory_order_relaxed);
    //  tail->popped.store(true, std::memory_order_release);
    //  if (prev)
    //    std::atomic_store_explicit(&prev->next, std::shared_ptr<Node>(), std::memory_order_release);
    //  //m_size.fetch_sub(1, std::memory_order_relaxed);
    //  value = tail->value;
    //  return true;
    //}

  private:
    std::shared_ptr<Node> m_head;
    std::shared_ptr<Node> m_tail;
    //std::atomic_int64_t m_size = 0;
  };

}

#endif
