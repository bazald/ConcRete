#ifndef ZENI_CONCURRENCY_EPOCH_LIST_HPP
#define ZENI_CONCURRENCY_EPOCH_LIST_HPP

#include "Queue.hpp"
#include "Shared_Ptr.hpp"

namespace Zeni::Concurrency {

  class Epoch_List {
    Epoch_List(const Epoch_List &) = delete;
    Epoch_List & operator=(const Epoch_List &) = delete;

  public:
    typedef Shared_Ptr<std::atomic_uint64_t> Token;

  private:
    struct Node : public Reclamation_Stack::Node {
      Node() = default;
      Node(const int64_t epoch_) : epoch(epoch_) {}

      std::atomic<Node *> next = nullptr;
      uint64_t epoch = 0;
    };

    struct Cursor {
      Cursor() = default;
      Cursor(Epoch_List * const epoch_list) : raw_cur(epoch_list->m_head.load(std::memory_order_relaxed)), raw_next(masked_cur->next.load(std::memory_order_relaxed)) {}

      // The Node at this Cursor appears to both (1) be marked for removal and to (2) follow a Node that is not marked for removal
      bool is_candidate_for_removal() const {
        return raw_cur == masked_cur && is_marked_for_deletion();
      }

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
        raw_next = masked_cur->next.load(std::memory_order_acquire);
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
    static const uint64_t epoch_increment = 2;

    Epoch_List() noexcept {
      std::atomic_thread_fence(std::memory_order_release);
    }

    ~Epoch_List() noexcept {
      std::atomic_thread_fence(std::memory_order_acquire);
      Node * head = m_head.load(std::memory_order_relaxed);
      while (head) {
        Node * next = reinterpret_cast<Node *>(uintptr_t(head->next.load(std::memory_order_relaxed)) & ~uintptr_t(0x1));
        delete head;
        head = next;
      }
    }

    bool empty() const {
      return m_head.load(std::memory_order_relaxed) == m_tail.load(std::memory_order_relaxed);
    }

    //int64_t size() const {
    //  return m_assigned_epochs.size();
    //}

    uint64_t front_and_acquire(const Token current_epoch) {
      //m_size.fetch_add(1, std::memory_order_relaxed);
      m_writers.fetch_add(1, std::memory_order_relaxed);

      Cursor cursor(this);
      while (try_removal(cursor));
      const int64_t front = cursor.masked_cur->epoch;

      m_acquires.push(current_epoch);
      continue_acquire(front, current_epoch);

      m_writers.fetch_sub(1, std::memory_order_relaxed);
      return front;
    }

    bool try_release(const Token epoch) {
      m_writers.fetch_add(1, std::memory_order_relaxed);

      Cursor cursor(this);
      while (try_removal(cursor));
      const int64_t front = cursor.masked_cur->epoch;

      continue_acquire(front, epoch);
      const bool success = try_erase(*Token::Lock(epoch));

      m_writers.fetch_sub(1, std::memory_order_relaxed);
      return success;
    }

  private:
    void continue_acquire(const uint64_t front, const Token epoch) {
      while (*Token::Lock(epoch) == 0) {
        Token current;
        if (!m_acquires.front(current))
          break;
        Token::Lock current_value(current);
        uint64_t value = *current_value;
        uint64_t next_assignable = m_next_assignable.load(std::memory_order_relaxed);
        if (value == 0 && current_value->compare_exchange_strong(value, next_assignable, std::memory_order_relaxed, std::memory_order_relaxed))
          value = next_assignable;
        if (value == next_assignable) {
          if (m_next_assignable.compare_exchange_strong(next_assignable, next_assignable + epoch_increment, std::memory_order_relaxed, std::memory_order_relaxed))
            next_assignable += epoch_increment;
        }

        Node * old_tail = m_tail.load(std::memory_order_acquire);
        Node * new_tail = nullptr;
        for (;;) {
          if (old_tail->epoch - front >= next_assignable - front)
            break;
          if (new_tail)
            new_tail->epoch = next_assignable;
          else
            new_tail = new Node(next_assignable);
          if (push_pointers(old_tail, new_tail)) {
            new_tail = nullptr;
            break;
          }
        }
        delete new_tail;

        m_acquires.try_pop_if_equals(current);
      }
    }

    bool try_erase(const uint64_t epoch) {
      m_writers.fetch_add(1, std::memory_order_relaxed);
      bool retry;
      do {
        Cursor cursor(this);
        retry = false;

        while (try_removal(cursor));
        const int64_t head_epoch = cursor.masked_cur->epoch;

        while (!cursor.is_end()) {
          if (cursor.is_marked_for_deletion()) {
            while (try_removal(cursor));
            continue;
          }

          if (cursor.masked_cur->epoch - head_epoch < epoch - head_epoch) {
            cursor.increment();
            continue;
          }
          else if (cursor.masked_cur->epoch - head_epoch > epoch - head_epoch)
            break;

          Node * const marked_next = reinterpret_cast<Node *>(uintptr_t(cursor.raw_next) | 0x1);
          if (cursor.masked_cur->next.compare_exchange_strong(cursor.masked_next, marked_next, std::memory_order_relaxed, std::memory_order_relaxed)) {
            cursor.raw_next = marked_next;
            try_removal(cursor);
            //m_size.fetch_sub(1, std::memory_order_relaxed);
            m_writers.fetch_sub(1, std::memory_order_relaxed);
            return true;
          }
          else {
            retry = true;
            break;
          }
        }
      } while (retry);
      m_writers.fetch_sub(1, std::memory_order_relaxed);
      return false;
    }

    // Return true if new tail pointer is used, otherwise false
    bool push_pointers(Node * &old_tail, Node * const new_tail) {
      Node * next = nullptr;
      if (old_tail->next.compare_exchange_weak(next, new_tail, std::memory_order_release, std::memory_order_relaxed)) {
        Node * old_tail_copy = old_tail;
        m_tail.compare_exchange_strong(old_tail_copy, new_tail, std::memory_order_relaxed, std::memory_order_relaxed);
        return true;
      }
      else {
        m_tail.compare_exchange_strong(old_tail, next, std::memory_order_release, std::memory_order_acquire);
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

      if (m_writers.load(std::memory_order_relaxed) == 1)
        delete old_cur;
      else
        Reclamation_Stacks::push(old_cur);

      return true;
    }

    std::atomic<Node *> m_head = new Node(1);
    std::atomic<Node *> m_tail = m_head.load(std::memory_order_relaxed);
    std::atomic_uint64_t m_next_assignable = 1;
    Queue<Token> m_acquires;
    //std::atomic_int64_t m_size = 0;
    std::atomic_int64_t m_writers = 0;
  };
  
}

#endif
