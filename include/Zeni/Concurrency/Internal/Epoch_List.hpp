#ifndef ZENI_CONCURRENCY_EPOCH_LIST_HPP
#define ZENI_CONCURRENCY_EPOCH_LIST_HPP

#include "Reclamation_Stacks.hpp"

#include <iostream>

namespace Zeni::Concurrency {

  class Epoch_List {
    Epoch_List(const Epoch_List &) = delete;
    Epoch_List & operator=(const Epoch_List &) = delete;

    struct Node : public Reclamation_Stack::Node {
      Node() = default;
      Node(const int64_t epoch_) : epoch(epoch_) {}

      std::atomic<Node *> next = nullptr;
      uint64_t epoch = 0;
    };

  public:
    static const uint64_t epoch_increment = 2;

    Epoch_List() noexcept {
      std::atomic_thread_fence(std::memory_order_release);
    }

    ~Epoch_List() noexcept {
      std::atomic_thread_fence(std::memory_order_acquire);
      Node * head = m_head.load(std::memory_order_relaxed);
      Node * tail = m_tail.load(std::memory_order_relaxed);
      while (head != tail) {
        Node * next = reinterpret_cast<Node *>(uintptr_t(head->next.load(std::memory_order_relaxed)) & ~uintptr_t(0x1));
        delete head;
        head = next;
      }
      delete head;
    }

    bool empty() const {
      return m_head.load(std::memory_order_relaxed) == m_tail.load(std::memory_order_relaxed);
    }

    std::pair<uint64_t, uint64_t> front_and_acquire() {
      const uint64_t acquired = acquire();

      m_writers.fetch_add(1, std::memory_order_relaxed);
      Node * masked_prev = nullptr;
      Node * raw_cur = m_head.load(std::memory_order_relaxed);
      Node * masked_cur = reinterpret_cast<Node *>(uintptr_t(raw_cur) & ~uintptr_t(0x1));
      Node * raw_next = masked_cur->next.load(std::memory_order_acquire);
      Node * masked_next = reinterpret_cast<Node *>(uintptr_t(raw_next) & ~uintptr_t(0x1));
      int64_t head_epoch = masked_cur->epoch;
      while (try_removal(masked_prev, raw_cur, masked_cur, raw_next, masked_next))
        head_epoch = masked_cur->epoch;
      m_writers.fetch_sub(1, std::memory_order_relaxed);

      return std::make_pair(head_epoch, acquired);
    }

    //int64_t size() const {
    //  return m_size.load(std::memory_order_relaxed);
    //}

    //int64_t usage() const {
    //  return m_usage.load(std::memory_order_relaxed);
    //}

    uint64_t acquire() {
      //m_size.fetch_add(1, std::memory_order_relaxed);
      //m_usage.fetch_add(1, std::memory_order_relaxed);
      m_writers.fetch_add(1, std::memory_order_relaxed);
      Node * old_tail = m_tail.load(std::memory_order_relaxed);
      Node * new_tail = new Node(old_tail->epoch + epoch_increment);
      while (!push_pointers(old_tail, new_tail))
        new_tail->epoch = old_tail->epoch + epoch_increment;
      m_writers.fetch_sub(1, std::memory_order_relaxed);
      return old_tail->epoch;
    }

    bool try_release(const uint64_t epoch) {
      m_writers.fetch_add(1, std::memory_order_relaxed);
      Node * masked_prev = nullptr;
      Node * raw_cur = m_head.load(std::memory_order_relaxed);
      Node * masked_cur = reinterpret_cast<Node *>(uintptr_t(raw_cur) & ~uintptr_t(0x1));
      Node * raw_next = masked_cur->next.load(std::memory_order_acquire);
      Node * masked_next = reinterpret_cast<Node *>(uintptr_t(raw_next) & ~uintptr_t(0x1));
      int64_t head_epoch = masked_cur->epoch;
      bool success = false;

      while (try_removal(masked_prev, raw_cur, masked_cur, raw_next, masked_next))
        head_epoch = masked_cur->epoch;

      for (;;) {
        if (masked_cur->epoch - head_epoch < epoch - head_epoch) {
          masked_prev = masked_cur;
          raw_cur = raw_next;
          masked_cur = masked_next;
          raw_next = masked_cur->next.load(std::memory_order_acquire);
          masked_next = reinterpret_cast<Node *>(uintptr_t(raw_next) & ~uintptr_t(0x1));
          while (try_removal(masked_prev, raw_cur, masked_cur, raw_next, masked_next));
          continue;
        }
        else if (masked_cur->epoch - head_epoch > epoch - head_epoch || !masked_next)
          break;

        if (raw_next == masked_next) {
          Node * const marked_next = reinterpret_cast<Node *>(uintptr_t(raw_next) | 0x1);
          if ((success = masked_cur->next.compare_exchange_strong(raw_next, marked_next, std::memory_order_relaxed, std::memory_order_relaxed))) {
            //m_size.fetch_sub(1, std::memory_order_relaxed);
          }
        }
        break;
      }
      m_writers.fetch_sub(1, std::memory_order_relaxed);
      return success;
    }

  private:
    // Return true if new tail pointer is used, otherwise false
    bool push_pointers(Node * &old_tail, Node * const new_tail) {
      Node * next = nullptr;
      if (old_tail->next.compare_exchange_weak(next, new_tail, std::memory_order_release, std::memory_order_relaxed)) {
        Node * old_tail_copy = old_tail;
        m_tail.compare_exchange_strong(old_tail_copy, new_tail, std::memory_order_relaxed, std::memory_order_relaxed);
        return true;
      }
      else {
        m_tail.compare_exchange_strong(old_tail, next, std::memory_order_relaxed, std::memory_order_relaxed);
        return false;
      }
    }

    // Return true if cur removed, otherwise false
    bool try_removal(Node * const masked_prev, Node * &raw_cur, Node * &masked_cur, Node * &raw_next, Node * &masked_next) {
      if (raw_cur != masked_cur || raw_next == masked_next)
        return false;

      Node * old_cur = masked_cur;
      if (!(masked_prev ? masked_prev->next : m_head).compare_exchange_strong(masked_cur, masked_next, std::memory_order_relaxed, std::memory_order_relaxed)) {
        masked_cur = old_cur;
        return false;
      }

      raw_cur = raw_next;
      masked_cur = masked_next;
      raw_next = masked_cur->next.load(std::memory_order_relaxed);
      masked_next = reinterpret_cast<Node *>(uintptr_t(raw_next) & ~uintptr_t(0x1));

      if (m_writers.load(std::memory_order_relaxed) == 1)
        delete old_cur;
      else
        Reclamation_Stacks::push(old_cur);
      //m_usage.fetch_sub(1, std::memory_order_relaxed);

      return true;
    }

    std::atomic<Node *> m_head = new Node();
    std::atomic<Node *> m_tail = m_head.load(std::memory_order_relaxed);
    //std::atomic_int64_t m_size = 0;
    //std::atomic_int64_t m_usage = 0;
    std::atomic_int64_t m_writers = 0;
  };

}

#endif
