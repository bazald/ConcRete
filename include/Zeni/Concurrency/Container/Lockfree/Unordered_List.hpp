#ifndef ZENI_CONCURRENCY_UNORDERED_LIST_HPP
#define ZENI_CONCURRENCY_UNORDERED_LIST_HPP

#include "../../Internal/Reclamation_Stacks.hpp"

namespace Zeni::Concurrency {

  template <typename TYPE>
  class Unordered_List {
    Unordered_List(const Unordered_List &) = delete;
    Unordered_List & operator=(const Unordered_List &) = delete;

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
      Cursor(Unordered_List * const unordered_list) : raw_cur(unordered_list->m_head.load(std::memory_order_relaxed)), raw_next(masked_cur->next.load(std::memory_order_relaxed)) {}

      Inner_Node * get_value_ptr() const {
        return masked_cur->value_ptr.load(std::memory_order_acquire);
      }

      // The Node at this Cursor appears to both (1) be marked for removal and to (2) follow a Node that is not marked for removal
      bool is_candidate_for_removal() const {
        return raw_cur == masked_cur && is_marked_for_deletion();
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
    Unordered_List() noexcept {
      std::atomic_thread_fence(std::memory_order_release);
    }

    ~Unordered_List() noexcept {
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
      return m_head.load(std::memory_order_relaxed) == m_tail.load(std::memory_order_relaxed);
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

    void push_front(const TYPE &value) {
      push_front(new Inner_Node(value));
    }

    void push_front(TYPE &&value) {
      push_front(new Inner_Node(std::move(value)));
    }

    void push_back(const TYPE &value) {
      push_back(new Inner_Node(value));
    }

    void push_back(TYPE &&value) {
      push_back(new Inner_Node(std::move(value)));
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
          if (!value_ptr || value_ptr->value != value) {
            cursor.increment();
            continue;
          }

          Node * const marked_next = reinterpret_cast<Node *>(uintptr_t(cursor.raw_next) | 0x1);
          if (cursor.masked_cur->next.compare_exchange_strong(cursor.raw_next, marked_next, std::memory_order_relaxed, std::memory_order_relaxed)) {
            //m_size.fetch_sub(1, std::memory_order_relaxed);
            m_writers.fetch_sub(1, std::memory_order_relaxed);
            return true;
          }
          else {
            //cursor.masked_next = reinterpret_cast<Node *>(uintptr_t(cursor.raw_next) & ~uintptr_t(0x1));
            retry = true;
            break;
          }
        }
      } while (retry);
      m_writers.fetch_sub(1, std::memory_order_relaxed);
      return false;
    }

  private:
    void push_front(Inner_Node * const value_ptr) {
      std::atomic_thread_fence(std::memory_order_release);
      //m_size.fetch_add(1, std::memory_order_relaxed);
      //m_usage.fetch_add(1, std::memory_order_relaxed);
      m_writers.fetch_add(1, std::memory_order_relaxed);
      Node * old_head = m_head.load(std::memory_order_relaxed);
      Node * new_head = new Node(old_head, value_ptr);
      while (!m_head.compare_exchange_weak(old_head, new_head, std::memory_order_relaxed, std::memory_order_relaxed))
        new_head->next.store(old_head, std::memory_order_relaxed);
      m_writers.fetch_sub(1, std::memory_order_relaxed);
    }

    void push_back(Inner_Node * const value_ptr) {
      std::atomic_thread_fence(std::memory_order_release);
      //m_size.fetch_add(1, std::memory_order_relaxed);
      //m_usage.fetch_add(1, std::memory_order_relaxed);
      m_writers.fetch_add(1, std::memory_order_relaxed);
      Node * old_tail = m_tail.load(std::memory_order_relaxed);
      Node * new_tail = new Node;
      for (;;) {
        Inner_Node * empty = nullptr;
        if (old_tail->value_ptr.compare_exchange_strong(empty, value_ptr, std::memory_order_relaxed, std::memory_order_relaxed)) {
          if (!push_pointers(old_tail, new_tail))
            delete new_tail;
          m_writers.fetch_sub(1, std::memory_order_relaxed);
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
      if (old_tail->next.compare_exchange_weak(next, new_tail, std::memory_order_relaxed, std::memory_order_relaxed)) {
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

    std::atomic<Node *> m_head = new Node();
    std::atomic<Node *> m_tail = m_head.load(std::memory_order_relaxed);
    //std::atomic_int64_t m_size = 0;
    //std::atomic_int64_t m_usage = 0;
    std::atomic_int64_t m_writers = 0;
  };

}

#endif
