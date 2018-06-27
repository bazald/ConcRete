#ifndef ZENI_CONCURRENCY_ANTIABLE_LIST_HPP
#define ZENI_CONCURRENCY_ANTIABLE_LIST_HPP

#include "Reclamation_Stacks.hpp"

#include <iostream>

namespace Zeni::Concurrency {

  template <typename TYPE>
  class Antiable_List {
    Antiable_List(const Antiable_List &) = delete;
    Antiable_List & operator=(const Antiable_List &) = delete;

    struct Inner_Node : public Reclamation_Stack::Node {
      Inner_Node() = default;
      Inner_Node(const TYPE &value_) : value(value_) {}
      Inner_Node(TYPE &&value_) : value(std::move(value_)) {}

      TYPE value;
    };

    struct Node : public Reclamation_Stack::Node {
      Node() = default;
      Node(Inner_Node * const value_ptr_, const int64_t inserted_) : value_ptr(value_ptr_), inserted(inserted_) {}
      Node(Node * const &next_, Inner_Node * const value_ptr_, const int64_t inserted_) : next(next_), value_ptr(value_ptr_), inserted(inserted_) {}
      Node(Node * &&next_, Inner_Node * const value_ptr_, const int64_t inserted_) : next(std::move(next_)), value_ptr(value_ptr_), inserted(inserted_) {}

      std::atomic<Node *> next = nullptr;
      std::atomic<Inner_Node *> value_ptr = nullptr;
      std::atomic<int64_t> instance_count = 1;
      int64_t inserted = 0;
      std::atomic<int64_t> removed = inserted;
    };

  public:
    Antiable_List() = default;

    ~Antiable_List() noexcept {
      Node * head = m_head.load(std::memory_order_acquire);
      Node * tail = m_tail.load(std::memory_order_acquire);
      while (head != tail) {
        Node * next = reinterpret_cast<Node *>(uintptr_t(head->next.load(std::memory_order_acquire)) & ~uintptr_t(0x1));
        Inner_Node * value_ptr = head->value_ptr.load(std::memory_order_acquire);
        delete head;
        delete value_ptr;
        head = next;
      }
      delete head;
    }

    //int64_t size() const {
    //  return m_size.load(std::memory_order_relaxed);
    //}

    //int64_t usage() const {
    //  return m_usage.load(std::memory_order_relaxed);
    //}

    void push_front(const int64_t earliest_epoch, const int64_t current_epoch, const TYPE &value) {
      push_front(earliest_epoch, current_epoch, new Inner_Node(value));
    }

    void push_front(const int64_t earliest_epoch, const int64_t current_epoch, TYPE &&value) {
      push_front(earliest_epoch, current_epoch, new Inner_Node(std::move(value)));
    }

    void push_back(const int64_t earliest_epoch, const int64_t current_epoch, const TYPE &value) {
      push_back(earliest_epoch, current_epoch, new Inner_Node(value));
    }

    void push_back(const int64_t earliest_epoch, const int64_t current_epoch, TYPE &&value) {
      push_back(earliest_epoch, current_epoch, new Inner_Node(std::move(value)));
    }

    bool try_erase(const int64_t earliest_epoch, const int64_t current_epoch, const TYPE &value) {
      m_writers.fetch_add(1, std::memory_order_release);
      Node * masked_prev = nullptr;
      Node * raw_cur = m_head.load(std::memory_order_acquire);
      Node * masked_cur = reinterpret_cast<Node *>(uintptr_t(raw_cur) & ~uintptr_t(0x1));
      Node * raw_next = masked_cur->next.load(std::memory_order_relaxed);
      Node * masked_next = reinterpret_cast<Node *>(uintptr_t(raw_next) & ~uintptr_t(0x1));
      bool success = false;

      while (try_removal(masked_prev, raw_cur, masked_cur, raw_next, masked_next));

      for (;;) {
        if (!masked_next)
          break;
        Inner_Node * const value_ptr = masked_cur->value_ptr.load(std::memory_order_acquire);
        if (raw_next != masked_next || value_ptr->value != value) {
          masked_prev = masked_cur;
          raw_cur = raw_next;
          masked_cur = masked_next;
          raw_next = masked_cur->next.load(std::memory_order_relaxed);
          masked_next = reinterpret_cast<Node *>(uintptr_t(raw_next) & ~uintptr_t(0x1));
          while (try_removal(masked_prev, raw_cur, masked_cur, raw_next, masked_next));
          continue;
        }

        Node * const marked_next = reinterpret_cast<Node *>(uintptr_t(raw_next) | 0x1);
        if ((success = masked_cur->next.compare_exchange_strong(raw_next, marked_next, std::memory_order_release, std::memory_order_relaxed))) {
          //m_size.fetch_sub(1, std::memory_order_relaxed);
          break;
        }
      }
      m_writers.fetch_sub(1, std::memory_order_relaxed);
      return success;
    }

  private:
    void push_front(const int64_t earliest_epoch, const int64_t current_epoch, Inner_Node * const value_ptr) {
      //m_size.fetch_add(1, std::memory_order_relaxed);
      //m_usage.fetch_add(1, std::memory_order_relaxed);
      m_writers.fetch_add(1, std::memory_order_release);
      Node * old_head = m_head.load(std::memory_order_relaxed);
      Node * new_head = new Node(old_head, value_ptr, current_epoch);
      while (!m_head.compare_exchange_weak(old_head, new_head, std::memory_order_release, std::memory_order_relaxed))
        new_head->next.store(old_head, std::memory_order_release);
      m_writers.fetch_sub(1, std::memory_order_release);
    }

    void push_back(const int64_t earliest_epoch, const int64_t current_epoch, Inner_Node * const value_ptr) {
      //m_size.fetch_add(1, std::memory_order_relaxed);
      //m_usage.fetch_add(1, std::memory_order_relaxed);
      m_writers.fetch_add(1, std::memory_order_release);
      Node * old_tail = m_tail.load(std::memory_order_relaxed);
      Node * new_tail = new Node;
      for (;;) {
        Inner_Node * empty = nullptr;
        if (old_tail->value_ptr.compare_exchange_strong(empty, value_ptr, std::memory_order_release, std::memory_order_relaxed)) {
          if (!push_pointers(old_tail, new_tail))
            delete new_tail;
          m_writers.fetch_sub(1, std::memory_order_release);
          return;
        }
        else {
          if (push_pointers(old_tail, new_tail))
            new_tail = new Node;
        }
      }
    }

    // Return true if new tail pointer is used, otherwise false
    bool push_pointers(Node * &old_tail, Node * const new_tail) {
      Node * next = nullptr;
      if (old_tail->next.compare_exchange_weak(next, new_tail, std::memory_order_release, std::memory_order_relaxed)) {
        Node * old_tail_copy = old_tail;
        m_tail.compare_exchange_strong(old_tail_copy, new_tail, std::memory_order_release, std::memory_order_relaxed);
        return true;
      }
      else {
        m_tail.compare_exchange_strong(old_tail, next, std::memory_order_release, std::memory_order_relaxed);
        return false;
      }
    }

    // Return true if cur removed, otherwise false
    bool try_removal(Node * const masked_prev, Node * &raw_cur, Node * &masked_cur, Node * &raw_next, Node * &masked_next) {
      if (raw_cur != masked_cur || raw_next == masked_next)
        return false;

      Node * old_cur = masked_cur;
      if (!(masked_prev ? masked_prev->next : m_head).compare_exchange_strong(masked_cur, masked_next, std::memory_order_release, std::memory_order_relaxed)) {
        masked_cur = old_cur;
        return false;
      }

      raw_cur = raw_next;
      masked_cur = masked_next;
      raw_next = masked_cur->next.load(std::memory_order_relaxed);
      masked_next = reinterpret_cast<Node *>(uintptr_t(raw_next) & ~uintptr_t(0x1));

      if (m_writers.load(std::memory_order_acquire) == 1) {
        delete old_cur->value_ptr.load(std::memory_order_relaxed);
        old_cur->value_ptr.store(nullptr, std::memory_order_release);
        delete old_cur;
      }
      else {
        Reclamation_Stacks::push(old_cur->value_ptr);
        Reclamation_Stacks::push(old_cur);
      }
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
