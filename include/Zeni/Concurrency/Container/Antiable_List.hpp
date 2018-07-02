#ifndef ZENI_CONCURRENCY_ANTIABLE_LIST_HPP
#define ZENI_CONCURRENCY_ANTIABLE_LIST_HPP

#include "../Internal/Reclamation_Stacks.hpp"
#include "../Internal/Epoch_List.hpp"

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
      Node(Inner_Node * const value_ptr_, const int64_t insertion_epoch_) : value_ptr(value_ptr_), creation_epoch(insertion_epoch_) {}
      Node(Node * const &next_, Inner_Node * const value_ptr_, const int64_t insertion_epoch_) : next(next_), value_ptr(value_ptr_), creation_epoch(insertion_epoch_) {}
      Node(Node * &&next_, Inner_Node * const value_ptr_, const int64_t insertion_epoch_) : next(std::move(next_)), value_ptr(value_ptr_), creation_epoch(insertion_epoch_) {}

      std::atomic<Node *> next = nullptr;
      std::atomic<Inner_Node *> value_ptr = nullptr;
      std::atomic<int64_t> instance_count = 1;
      std::atomic<int64_t> creation_epoch = 0;
      std::atomic<int64_t> access_epoch = creation_epoch.load(std::memory_order_relaxed);
    };

    struct Cursor {
      Cursor() = default;
      Cursor(Antiable_List * const ordered_list) : raw_cur(ordered_list->m_head.load(std::memory_order_relaxed)), raw_next(masked_cur->next.load(std::memory_order_relaxed)) {}

      Inner_Node * get_value_ptr() const {
        return masked_cur ? masked_cur->value_ptr.load(std::memory_order_acquire) : nullptr;
      }

      // The Node at this Cursor appears to both (1) be marked for removal and to (2) follow a Node that is not marked for removal
      bool is_candidate_for_removal(const int64_t earliest_epoch) const {
        if (raw_cur != masked_cur || raw_next == masked_next)
          return false;
        const int64_t creation_epoch = masked_cur->creation_epoch.load(std::memory_order_relaxed);
        const int64_t deletion_epoch = masked_cur->access_epoch.load(std::memory_order_relaxed);
        return deletion_epoch - creation_epoch < earliest_epoch - creation_epoch;
      }

      bool is_marked_for_deletion() const {
        return masked_cur && masked_cur->instance_count.load(std::memory_order_relaxed) == 0;
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
    Antiable_List() noexcept {
      std::atomic_thread_fence(std::memory_order_release);
    }

    ~Antiable_List() noexcept {
      std::atomic_thread_fence(std::memory_order_acquire);
      Node * head = m_head.load(std::memory_order_relaxed);
      while (head) {
        Node * next = reinterpret_cast<Node *>(uintptr_t(head->next.load(std::memory_order_relaxed)) & ~uintptr_t(0x1));
        Inner_Node * value_ptr = head->value_ptr.load(std::memory_order_relaxed);
        delete head;
        delete value_ptr;
        head = next;
      }
    }

    bool empty() const {
      return m_head.load(std::memory_order_relaxed) != nullptr;
    }

    //int64_t size() const {
    //  return m_size.load(std::memory_order_relaxed);
    //}

    //int64_t usage() const {
    //  return m_usage.load(std::memory_order_relaxed);
    //}

    void insert(const std::shared_ptr<Epoch_List> &epoch_list, const TYPE &value) {
      //m_size.fetch_add(1, std::memory_order_relaxed);
      //m_usage.fetch_add(1, std::memory_order_relaxed);
      m_writers.fetch_add(1, std::memory_order_relaxed);
      int64_t earliest_epoch, current_epoch;
      {
        const auto &[earliest_epoch_, current_epoch_] = epoch_list->front_and_acquire();
        earliest_epoch = earliest_epoch_;
        current_epoch = current_epoch_;
      }

      Node * const new_value = new Node(new Inner_Node(value), current_epoch);
      std::atomic_thread_fence(std::memory_order_release);
      for(;;) {
        Cursor last_unmarked;
        Cursor cursor(this);
        Node * head = cursor.raw_cur;

        while (try_removal(epoch_list, earliest_epoch, current_epoch, cursor));

        for (;;) {
          if (cursor.is_marked_for_deletion()) {
            while (try_removal(epoch_list, earliest_epoch, current_epoch, cursor));
            continue;
          }

          Inner_Node * value_ptr = cursor.get_value_ptr();
          if (!cursor.is_end() && (!value_ptr || value_ptr->value < value)) {
            last_unmarked = cursor;
            cursor.increment();
            continue;
          }

          if (value_ptr && value_ptr->value == value) {
            int64_t instance_count = cursor.masked_cur->instance_count.load(std::memory_order_relaxed);
            while (instance_count) {
              if (cursor.masked_cur->instance_count.compare_exchange_strong(instance_count, instance_count + 1, std::memory_order_relaxed, std::memory_order_relaxed)) {
                //m_size.fetch_sub(1, std::memory_order_relaxed);
                epoch_list->try_release(current_epoch);
                m_writers.fetch_sub(1, std::memory_order_relaxed);
                delete new_value->value_ptr.load(std::memory_order_relaxed);
                delete new_value;
                return;
              }
            }
          }

          if (last_unmarked.masked_cur) {
            new_value->next.store(last_unmarked.masked_next, std::memory_order_relaxed);
            if (last_unmarked.masked_cur->next.compare_exchange_strong(last_unmarked.masked_next, new_value, std::memory_order_relaxed, std::memory_order_relaxed)) {
              //m_size.fetch_sub(1, std::memory_order_relaxed);
              epoch_list->try_release(current_epoch);
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
              epoch_list->try_release(current_epoch);
              m_writers.fetch_sub(1, std::memory_order_relaxed);
              return;
            }
            else
              break;
          }
        }
      }
    }

    bool try_erase(const std::shared_ptr<Epoch_List> &epoch_list, const TYPE &value) {
      m_writers.fetch_add(1, std::memory_order_relaxed);
      int64_t earliest_epoch, current_epoch;
      {
        const auto &[earliest_epoch_, current_epoch_] = epoch_list->front_and_acquire();
        earliest_epoch = earliest_epoch_;
        current_epoch = current_epoch_;
      }

      bool retry;
      do {
        Cursor cursor(this);
        retry = false;

        while (try_removal(epoch_list, earliest_epoch, current_epoch, cursor));

        while (!cursor.is_end()) {
          if (cursor.is_marked_for_deletion()) {
            while (try_removal(epoch_list, earliest_epoch, current_epoch, cursor));
            continue;
          }

          Inner_Node * const value_ptr = cursor.get_value_ptr();
          if (!value_ptr || value_ptr->value < value) {
            cursor.increment();
            continue;
          }
          else if (value_ptr->value > value)
            break;

          int64_t instance_count = cursor.masked_cur->instance_count.load(std::memory_order_relaxed);
          while (instance_count) {
            if (cursor.masked_cur->instance_count.compare_exchange_strong(instance_count, instance_count - 1, std::memory_order_relaxed, std::memory_order_relaxed)) {
              if (--instance_count == 0)
                try_removal(epoch_list, earliest_epoch, current_epoch, cursor);
              //m_size.fetch_sub(1, std::memory_order_relaxed);
              epoch_list->try_release(current_epoch);
              m_writers.fetch_sub(1, std::memory_order_relaxed);
              return true;
            }
          }

          retry = true;
        }
      } while (retry);

      epoch_list->try_release(current_epoch);
      m_writers.fetch_sub(1, std::memory_order_relaxed);
      return false;
    }

  private:
    // Return true if cur removed, otherwise false
    bool try_removal(const std::shared_ptr<Epoch_List> &epoch_list, int64_t &earliest_epoch, int64_t &current_epoch, Cursor &cursor) {
      if (!cursor.is_marked_for_deletion())
        return false;

      {
        int64_t access_epoch = cursor.masked_cur->access_epoch.load(std::memory_order_relaxed);
        while (!(access_epoch & 0x1)) {
          if (access_epoch - earliest_epoch > current_epoch - earliest_epoch) {
            epoch_list->try_release(current_epoch);
            const auto &[earliest_epoch_, current_epoch_] = epoch_list->front_and_acquire();
            earliest_epoch = earliest_epoch_;
            current_epoch = current_epoch_;
          }

          if (cursor.masked_cur->access_epoch.compare_exchange_strong(access_epoch, current_epoch | 0x1, std::memory_order_relaxed, std::memory_order_relaxed))
            break;
        }
      }

      while (!(uintptr_t(cursor.raw_next) & 0x1)) {
        Node * const marked_next = reinterpret_cast<Node *>(uintptr_t(cursor.raw_next) | 0x1);
        if (cursor.masked_cur->next.compare_exchange_strong(cursor.raw_next, marked_next, std::memory_order_relaxed, std::memory_order_relaxed)) {
          cursor.masked_next = cursor.raw_next;
          cursor.raw_next = marked_next;
        }
        else
          cursor.masked_next = reinterpret_cast<Node *>(uintptr_t(cursor.raw_next) & ~uintptr_t(0x1));
      }

      if (!cursor.is_candidate_for_removal(earliest_epoch)) {
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
      //m_usage.fetch_sub(1, std::memory_order_relaxed);

      return true;
    }

    std::atomic<Node *> m_head = new Node();
    //std::atomic_int64_t m_size = 0;
    //std::atomic_int64_t m_usage = 0;
    std::atomic_int64_t m_writers = 0;
  };

}

#endif
