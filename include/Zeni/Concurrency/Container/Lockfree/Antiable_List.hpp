#ifndef ZENI_CONCURRENCY_ANTIABLE_LIST_HPP
#define ZENI_CONCURRENCY_ANTIABLE_LIST_HPP

#include "Epoch_List.hpp"

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
      Node(Inner_Node * const value_ptr_, const bool insertion_, const uint64_t creation_epoch_) : value_ptr(value_ptr_), instance_count(insertion_ ? 1 : -1), creation_epoch(creation_epoch_), insertion(insertion_) {}
      Node(Node * const &next_, Inner_Node * const value_ptr_, const bool insertion_, const uint64_t creation_epoch_) : next(next_), value_ptr(value_ptr_), instance_count(insertion_ ? 1 : -1), creation_epoch(creation_epoch_), insertion(insertion_) {}
      Node(Node * &&next_, Inner_Node * const value_ptr_, const bool insertion_, const uint64_t creation_epoch_) : next(std::move(next_)), value_ptr(value_ptr_), instance_count(insertion_ ? 1 : -1), creation_epoch(creation_epoch_), insertion(insertion_) {}

      std::atomic<Node *> next = nullptr;
      std::atomic<Inner_Node *> value_ptr = nullptr;
      std::atomic<uint64_t> instance_count = 1;
      Epoch_List::Token creation_epoch = Zeni::Concurrency::Epoch_List::Token(new std::atomic_uint64_t(0));
      Epoch_List::Token deletion_epoch = Zeni::Concurrency::Epoch_List::Token(new std::atomic_uint64_t(0));
      bool insertion = false;
    };

    struct Cursor {
      Cursor() = default;
      Cursor(Antiable_List * const antiable_list) : raw_cur(antiable_list->m_head.load(std::memory_order_relaxed)), raw_next(masked_cur ? masked_cur->next.load(std::memory_order_relaxed) : nullptr) {}

      Inner_Node * get_value_ptr() const {
        return masked_cur ? masked_cur->value_ptr.load(std::memory_order_acquire) : nullptr;
      }

      bool is_candidate_for_removal(const uint64_t earliest_epoch) const {
        if (raw_cur != masked_cur || raw_next == masked_next)
          return false;
        const uint64_t creation_epoch = masked_cur->creation_epoch.load(std::memory_order_relaxed);
        const uint64_t deletion_epoch = masked_cur->access_epoch.load(std::memory_order_relaxed);
        if (!(deletion_epoch & 0x1))
          return false;
        return deletion_epoch - earliest_epoch < deletion_epoch - creation_epoch;
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

    enum class Mode { Erase, Insert };

  public:
    typedef TYPE value_type;
    typedef const value_type & reference;

    class const_iterator {
    public:
      typedef std::forward_iterator_tag iterator_category;
      typedef TYPE value_type;
      typedef const value_type & reference;

      const_iterator() = default;

      const_iterator(const uint64_t current_epoch)
        : m_current_epoch(current_epoch)
      {
      }

      const_iterator(const uint64_t current_epoch, Node * const node)
        : m_current_epoch(current_epoch),
        m_node(node)
      {
        validate();
      }

      reference operator*() const { return m_node->value_ptr.load(std::memory_order_relaxed)->value; }

      const_iterator next() const {
        return ++const_iterator(*this);
      }

      const_iterator & operator++() {
        m_node = reinterpret_cast<Node *>(uintptr_t(m_node->next.load(std::memory_order_relaxed)) & ~uintptr_t(0x1));
        validate();
        return *this;
      }
      const_iterator operator++(int) {
        const_iterator rv(*this);
        ++*this;
        return rv;
      }

      bool operator==(const const_iterator &rhs) const {
        return m_node == rhs.m_node && m_current_epoch == rhs.m_current_epoch;
      }
      bool operator!=(const const_iterator &rhs) const {
        return m_node != rhs.m_node || m_current_epoch != rhs.m_current_epoch;
      }

    private:
      void validate() {
        for (;;) {
          if (!m_node)
            break;
          if (!m_node->insertion) {
            m_node = reinterpret_cast<Node *>(uintptr_t(m_node->next.load(std::memory_order_relaxed)) & ~uintptr_t(0x1));
            continue;
          }
          const uint64_t creation_epoch = m_node->creation_epoch.load(std::memory_order_relaxed);
          uint64_t access_epoch = m_node->access_epoch.load(std::memory_order_relaxed);
          bool outside_range;
          for (;;) {
            outside_range = access_epoch - creation_epoch < access_epoch - m_current_epoch || ((access_epoch & 0x1) && access_epoch - creation_epoch < m_current_epoch - creation_epoch);
            //if (m_node->access_epoch.compare_exchange_weak(access_epoch, m_current_epoch, std::memory_order_relaxed, std::memory_order_relaxed))
              break;
          }
          if (outside_range) {
            m_node = reinterpret_cast<Node *>(uintptr_t(m_node->next.load(std::memory_order_relaxed)) & ~uintptr_t(0x1));
            continue;
          }
          std::atomic_thread_fence(std::memory_order_acquire);
          break;
        }
      }

      uint64_t m_current_epoch = 0;
      Node * m_node = nullptr;
    };

    const_iterator cbegin(const uint64_t current_epoch) const {
      return const_iterator(current_epoch, m_head.load(std::memory_order_relaxed));
    }

    const_iterator cend(const uint64_t current_epoch) const {
      return const_iterator(current_epoch);
    }

    const_iterator begin(const uint64_t current_epoch) const {
      return cbegin(current_epoch);
    }

    const_iterator end(const uint64_t current_epoch) const {
      return cend(current_epoch);
    }

    Antiable_List() noexcept {
      std::atomic_thread_fence(std::memory_order_release);
    }

    ~Antiable_List() noexcept {
      std::atomic_thread_fence(std::memory_order_acquire);
      Node * head = m_head.load(std::memory_order_relaxed);
      while (head) {
        Node * const next = reinterpret_cast<Node *>(uintptr_t(head->next.load(std::memory_order_relaxed)) & ~uintptr_t(0x1));
        Inner_Node * const value_ptr = head->value_ptr.load(std::memory_order_relaxed);
        delete head;
        delete value_ptr;
        head = next;
      }
    }

    bool empty() const {
      return m_head.load(std::memory_order_relaxed) == nullptr;
    }

    int64_t size() const {
      return m_size.load(std::memory_order_relaxed);
    }

    int64_t usage() const {
      return m_usage.load(std::memory_order_relaxed);
    }

    /// Return true if it decrements the count to 0, otherwise false
    bool erase(const std::shared_ptr<Epoch_List> epoch_list, const TYPE &value) {
      return access(epoch_list, value, Mode::Erase) == 0;
    }

    /// Return true if it increments the count to 1, otherwise false
    bool insert(const std::shared_ptr<Epoch_List> epoch_list, const TYPE &value) {
      return access(epoch_list, value, Mode::Insert) == 1;
    }

  private:
    /// Return true if it increments the count to 1, otherwise false
    bool access(const std::shared_ptr<Epoch_List> epoch_list, const TYPE &value, const Mode mode) {
      m_writers.fetch_add(1, std::memory_order_relaxed);
      uint64_t earliest_epoch = epoch_list->front();

      Node * new_value = nullptr;
      for (;;) {
        Cursor last_unmarked;
        Cursor cursor(this);
        Node * head = cursor.raw_cur;

        while (try_removal(epoch_list, earliest_epoch, cursor));

        for (;;) {
          if (cursor.is_marked_for_deletion()) {
            while (try_removal(epoch_list, earliest_epoch, cursor));
            continue;
          }

          Inner_Node * value_ptr = cursor.get_value_ptr();
          if (!cursor.is_end() && (!value_ptr || value_ptr->value < value)) {
            last_unmarked = cursor;
            cursor.increment();
            continue;
          }

          if (value_ptr && value_ptr->value == value) {
            uint64_t instance_count = cursor.masked_cur->instance_count.load(std::memory_order_relaxed);
            const uint64_t instance_count_increment = mode == Mode::Insert ? 1 : -1;
            while (instance_count) {
              if (cursor.masked_cur->instance_count.compare_exchange_strong(instance_count, instance_count + instance_count_increment, std::memory_order_relaxed, std::memory_order_relaxed)) {
                if (instance_count > 0)
                  m_size.fetch_add(instance_count_increment, std::memory_order_relaxed);
                instance_count += instance_count_increment;
                if (instance_count == 0)
                  m_usage.fetch_sub(1, std::memory_order_relaxed);
                if (instance_count == 0)
                  try_removal(epoch_list, earliest_epoch, cursor);
                epoch_list->try_release(current_epoch);
                m_writers.fetch_sub(1, std::memory_order_relaxed);
                if (new_value)
                  delete new_value->value_ptr.load(std::memory_order_relaxed);
                delete new_value;
                return mode == Mode::Erase && instance_count == 0;
              }
            }
          }

          if (!new_value) {
            new_value = new Node(new Inner_Node(value), mode == Mode::Insert, current_epoch);
            std::atomic_thread_fence(std::memory_order_release);
          }

          new_value->next.store(last_unmarked.masked_cur ? last_unmarked.masked_next : head, std::memory_order_relaxed);
          if ((last_unmarked.masked_cur ? last_unmarked.masked_cur->next : m_head).compare_exchange_strong(last_unmarked.masked_cur ? last_unmarked.masked_next : head, new_value, std::memory_order_relaxed, std::memory_order_relaxed)) {
            if (mode == Mode::Insert)
              m_size.fetch_add(1, std::memory_order_relaxed);
            m_usage.fetch_add(1, std::memory_order_relaxed);
            epoch_list->try_release(current_epoch);
            m_writers.fetch_sub(1, std::memory_order_relaxed);
            return mode == Mode::Insert;
          }
          else
            break;
        }
      }
    }

    // Return true if cur removed, otherwise false
    bool try_removal(const std::shared_ptr<Epoch_List> epoch_list, uint64_t &earliest_epoch, Cursor &cursor) {
      if (!cursor.is_marked_for_deletion())
        return false;

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

      Reclamation_Stacks::push(old_cur->value_ptr);
      Reclamation_Stacks::push(old_cur);

      return true;
    }

    std::atomic<Node *> m_head = nullptr;
    std::atomic_int64_t m_size = 0;
    std::atomic_int64_t m_usage = 0;
    std::atomic_int64_t m_writers = 0;
  };

}

#endif
