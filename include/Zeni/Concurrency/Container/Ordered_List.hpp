#ifndef ZENI_CONCURRENCY_ORDERED_LIST_HPP
#define ZENI_CONCURRENCY_ORDERED_LIST_HPP

#include "../Internal/Reclamation_Stacks.hpp"

#include <iostream>

namespace Zeni::Concurrency {

  template <typename TYPE>
  class Ordered_List {
    Ordered_List(const Ordered_List &) = delete;
    Ordered_List & operator=(const Ordered_List &) = delete;

    struct Inner_Node : public Reclamation_Stack::Node {
      Inner_Node() = default;
      Inner_Node(const TYPE &value_) : value(value_) {}
      Inner_Node(TYPE &&value_) : value(std::move(value_)) {}

      TYPE value;
    };

    struct Node : public Reclamation_Stack::Node {
      Node() = default;
      Node(Inner_Node * const value_ptr_) : value_ptr(value_ptr_) {}
      Node(Node * const &next_, Inner_Node * const value_ptr_) : next(next_), value_ptr(value_ptr_) {}
      Node(Node * &&next_, Inner_Node * const value_ptr_) : next(std::move(next_)), value_ptr(value_ptr_) {}

      std::atomic<Node *> next = nullptr;
      std::atomic<Inner_Node *> value_ptr = nullptr;
    };

    struct Cursor {
      Cursor() = default;
      Cursor(Ordered_List * const ordered_list) : raw_cur(ordered_list->m_head.load(std::memory_order_acquire)), raw_next(masked_cur->next.load(std::memory_order_relaxed)) {}

      Inner_Node * get_value_ptr() const {
        return masked_cur->value_ptr.load(std::memory_order_acquire);
      }

      // The Node at this Cursor appears to both (1) be marked for removal and to (2) follow a Node that is not marked for removal
      bool is_candidate_for_removal() const {
        return raw_cur == masked_cur && is_marked_for_deletion() && get_value_ptr() == nullptr;
      }

      // The Node at this Cursor appears to both (1) be marked for removal and to (2) follow a Node that is not marked for removal
      bool is_marked_for_deletion() const {
        return raw_next != masked_next;
      }

      bool is_end() const {
        return !masked_next;
      }

      bool increment() {
        if (is_end())
          return false;
        prev = masked_cur;
        raw_cur = raw_next;
        masked_cur = masked_next;
        raw_next = masked_cur->next.load(std::memory_order_relaxed);
        masked_next = reinterpret_cast<Node *>(uintptr_t(raw_next) & ~uintptr_t(0x1));
        return true;
      }

      Node * prev = nullptr;
      Node * raw_cur = nullptr;
      Node * masked_cur = reinterpret_cast<Node *>(uintptr_t(raw_cur) & ~uintptr_t(0x1));
      Node * raw_next = nullptr;
      Node * masked_next = reinterpret_cast<Node *>(uintptr_t(raw_next) & ~uintptr_t(0x1));
    };

  public:
    Ordered_List() = default;

    ~Ordered_List() noexcept {
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

    bool empty() const {
      return m_head.load(std::memory_order_acquire) == m_tail.load(std::memory_order_acquire);
    }

    //int64_t size() const {
    //  return m_size.load(std::memory_order_relaxed);
    //}

    //int64_t usage() const {
    //  return m_usage.load(std::memory_order_relaxed);
    //}

    void insert(const TYPE &value) {
      //m_size.fetch_add(1, std::memory_order_relaxed);
      //m_usage.fetch_add(1, std::memory_order_relaxed);
      m_writers.fetch_add(1, std::memory_order_release);

      Node * const new_value = new Node(new Inner_Node(value));
      bool retry;
      do {
        Cursor last_unmarked;
        Cursor cursor(this);
        Node * head = cursor.raw_cur;
        retry = false;

        while (try_removal(cursor));

        for(;;) {
          if (cursor.is_marked_for_deletion()) {
            while (try_removal(cursor));
            continue;
          }

          Inner_Node * value_ptr = cursor.get_value_ptr();
          if (!cursor.is_end() && (!value_ptr || value_ptr->value < value)) {
            last_unmarked = cursor;
            cursor.increment();
            continue;
          }

          if (last_unmarked.masked_cur) {
            new_value->next.store(last_unmarked.masked_next, std::memory_order_release);
            if (last_unmarked.masked_cur->next.compare_exchange_strong(last_unmarked.masked_next, new_value, std::memory_order_release, std::memory_order_acquire)) {
              //m_size.fetch_sub(1, std::memory_order_relaxed);
              m_writers.fetch_sub(1, std::memory_order_relaxed);
              return;
            }
            else {
              cursor.prev = last_unmarked.masked_cur;
              cursor.raw_cur = last_unmarked.masked_next;
              cursor.masked_cur = reinterpret_cast<Node *>(uintptr_t(cursor.raw_cur) & ~uintptr_t(0x1));
              if (cursor.raw_cur == cursor.masked_cur) {
                cursor.raw_next = cursor.masked_cur->next.load(std::memory_order_relaxed);
                cursor.masked_next = reinterpret_cast<Node *>(uintptr_t(cursor.raw_next) & ~uintptr_t(0x1));
                continue;
              }
              else {
                retry = true;
                break;
              }
            }
          }
          else {
            new_value->next.store(head, std::memory_order_release);
            if (m_head.compare_exchange_strong(head, new_value, std::memory_order_release, std::memory_order_relaxed)) {
              //m_size.fetch_sub(1, std::memory_order_relaxed);
              m_writers.fetch_sub(1, std::memory_order_relaxed);
              return;
            }
            else {
              retry = true;
              break;
            }
          }
        }
      } while (retry);

      m_writers.fetch_sub(1, std::memory_order_release);
    }

    bool try_erase(const TYPE &value) {
      m_writers.fetch_add(1, std::memory_order_release);
      bool retry;
      do {
        Cursor cursor(this);
        retry = false;

        while (try_removal(cursor));

        while (!cursor.is_end()) {
          if (cursor.is_marked_for_deletion()) {
            while (try_removal(cursor));
            continue;
          }

          Inner_Node * const value_ptr = cursor.get_value_ptr();
          if (!value_ptr || value_ptr->value < value) {
            cursor.increment();
            continue;
          }
          else if (value_ptr->value > value)
            break;

          Node * const marked_next = reinterpret_cast<Node *>(uintptr_t(cursor.raw_next) | 0x1);
          if (cursor.masked_cur->next.compare_exchange_strong(cursor.raw_next, marked_next, std::memory_order_release, std::memory_order_relaxed)) {
            //m_size.fetch_sub(1, std::memory_order_relaxed);
            m_writers.fetch_sub(1, std::memory_order_relaxed);
            return true;
          }
          else
            retry = true;
        }
      } while (retry);
      m_writers.fetch_sub(1, std::memory_order_relaxed);
      return false;
    }

  private:
    // Return true if cur removed, otherwise false
    bool try_removal(Cursor &cursor) {
      if (!cursor.is_marked_for_deletion())
        return false;
      else if (!cursor.is_candidate_for_removal()) {
        cursor.increment();
        return false;
      }

      Node * const old_cur = cursor.masked_cur;
      if (!(cursor.prev ? cursor.prev->next : m_head).compare_exchange_strong(cursor.masked_cur, cursor.masked_next, std::memory_order_release, std::memory_order_relaxed)) {
        cursor.masked_cur = old_cur;
        cursor.increment();
        return false;
      }

      cursor.increment();

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
