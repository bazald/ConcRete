#ifndef ZENI_CONCURRENCY_ORDERED_LIST_HPP
#define ZENI_CONCURRENCY_ORDERED_LIST_HPP

#include "../../Internal/Reclamation_Stacks.hpp"

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

      ZENI_CONCURRENCY_CACHE_ALIGN std::atomic<Node *> next = nullptr;
      ZENI_CONCURRENCY_CACHE_ALIGN std::atomic<Inner_Node *> value_ptr = nullptr;
    };

    struct Cursor {
      Cursor() = default;
      Cursor(Ordered_List * const ordered_list) : raw_cur(ordered_list->m_head.load(std::memory_order_relaxed)), raw_next(masked_cur->next.load(std::memory_order_relaxed)) {}

      Inner_Node * get_value_ptr() const {
        return masked_cur ? masked_cur->value_ptr.load(std::memory_order_acquire) : nullptr;
      }

      // The Node at this Cursor appears to both (1) be marked for removal and to (2) follow a Node that is not marked for removal
      bool is_candidate_for_removal() const {
        return raw_cur == masked_cur && is_marked_for_deletion() && get_value_ptr() == nullptr;
      }

      bool is_marked_for_deletion() const {
        return raw_next != masked_next;
      }

      bool is_end() const {
        return !masked_cur;
      }

      bool increment() {
        if (is_end())
          return false;
        prev = masked_cur;
        raw_cur = raw_next;
        masked_cur = masked_next;
        if (masked_cur) {
          raw_next = masked_cur->next.load(std::memory_order_relaxed);
          masked_next = reinterpret_cast<Node *>(uintptr_t(raw_next) & ~uintptr_t(0x1));
        }
        else {
          raw_next = nullptr;
          masked_next = nullptr;
        }
        return true;
      }

      Node * prev = nullptr;
      Node * raw_cur = nullptr;
      Node * masked_cur = reinterpret_cast<Node *>(uintptr_t(raw_cur) & ~uintptr_t(0x1));
      Node * raw_next = nullptr;
      Node * masked_next = reinterpret_cast<Node *>(uintptr_t(raw_next) & ~uintptr_t(0x1));
    };

  public:
    Ordered_List() noexcept {
      std::atomic_thread_fence(std::memory_order_release);
    }

    ~Ordered_List() noexcept {
      std::atomic_thread_fence(std::memory_order_acquire);
      Node * head = m_head.load(std::memory_order_relaxed);
      while (head) {
        Node * const next = reinterpret_cast<Node *>(uintptr_t(head->next.load(std::memory_order_relaxed)) & ~uintptr_t(0x1));
        Inner_Node * value_ptr = head->value_ptr.load(std::memory_order_relaxed);
        delete head;
        delete value_ptr;
        head = next;
      }
    }

    bool empty() const {
      return m_head.load(std::memory_order_relaxed) == nullptr;
    }

    //int64_t size() const {
    //  return m_size.load(std::memory_order_relaxed);
    //}

    //int64_t usage() const {
    //  return m_usage.load(std::memory_order_relaxed);
    //}

    bool front(TYPE &value) {
      m_writers.fetch_add(1, std::memory_order_relaxed);
      Cursor cursor(this);
      while (try_removal(cursor));
      while (!cursor.is_end()) {
        if (cursor.is_marked_for_deletion()) {
          while (try_removal(cursor));
          continue;
        }
        Inner_Node * const value_ptr = cursor.get_value_ptr();
        if (!value_ptr) {
          cursor.increment();
          continue;
        }
        value = value_ptr->value;
        m_writers.fetch_sub(1, std::memory_order_relaxed);
        return true;
      }
      m_writers.fetch_sub(1, std::memory_order_relaxed);
      return false;
    }

    void insert(const TYPE &value) {
      //m_size.fetch_add(1, std::memory_order_relaxed);
      //m_usage.fetch_add(1, std::memory_order_relaxed);
      m_writers.fetch_add(1, std::memory_order_relaxed);

      Node * const new_value = new Node(new Inner_Node(value));
      std::atomic_thread_fence(std::memory_order_release);
      for(;;) {
        Cursor last_unmarked;
        Cursor cursor(this);
        Node * head = cursor.raw_cur;

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
            new_value->next.store(last_unmarked.masked_next, std::memory_order_relaxed);
            if (last_unmarked.masked_cur->next.compare_exchange_strong(last_unmarked.masked_next, new_value, std::memory_order_relaxed, std::memory_order_relaxed)) {
              //m_size.fetch_sub(1, std::memory_order_relaxed);
              m_writers.fetch_sub(1, std::memory_order_relaxed);
              return;
            }
            else
              break;
          }
          else {
            new_value->next.store(head, std::memory_order_relaxed);
            if (m_head.compare_exchange_strong(head, new_value, std::memory_order_relaxed, std::memory_order_relaxed)) {
              //m_size.fetch_sub(1, std::memory_order_relaxed);
              m_writers.fetch_sub(1, std::memory_order_relaxed);
              return;
            }
            else
              break;
          }
        }
      }
    }

    bool try_erase(const TYPE &value) {
      m_writers.fetch_add(1, std::memory_order_relaxed);
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
          if (cursor.masked_cur->next.compare_exchange_strong(cursor.raw_next, marked_next, std::memory_order_relaxed, std::memory_order_relaxed)) {
            cursor.raw_next = marked_next;
            try_removal(cursor);
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
      if (!(cursor.prev ? cursor.prev->next : m_head).compare_exchange_strong(cursor.masked_cur, cursor.masked_next, std::memory_order_relaxed, std::memory_order_relaxed)) {
        cursor.masked_cur = old_cur;
        cursor.increment();
        return false;
      }
      else if (!cursor.prev)
        cursor.masked_cur = nullptr; // Ensure that cursor.increment() results in null cursor.prev

      cursor.increment();

      if (m_writers.load(std::memory_order_relaxed) == 1) {
        delete old_cur->value_ptr.load(std::memory_order_relaxed);
        old_cur->value_ptr.store(nullptr, std::memory_order_relaxed);
        delete old_cur;
      }
      else {
        Reclamation_Stacks::push(old_cur->value_ptr);
        Reclamation_Stacks::push(old_cur);
      }

      return true;
    }

    ZENI_CONCURRENCY_CACHE_ALIGN std::atomic<Node *> m_head = new Node();
    //ZENI_CONCURRENCY_CACHE_ALIGN std::atomic_int64_t m_size = 0;
    //ZENI_CONCURRENCY_CACHE_ALIGN std::atomic_int64_t m_usage = 0;
    ZENI_CONCURRENCY_CACHE_ALIGN std::atomic_int64_t m_writers = 0;
  };

}

#endif
