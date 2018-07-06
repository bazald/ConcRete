#ifndef ZENI_CONCURRENCY_ANTIABLE_LIST_HPP
#define ZENI_CONCURRENCY_ANTIABLE_LIST_HPP

#include "Epoch_List.hpp"

namespace Zeni::Concurrency {

  template <typename TYPE>
  class Antiable_List {
    Antiable_List(const Antiable_List &) = delete;
    Antiable_List & operator=(const Antiable_List &) = delete;

    struct Node {
      Node() = default;
      Node(const TYPE &value_, const bool insertion_, const int64_t creation_epoch_) : value(value_), instance_count(insertion_ ? 1 : -1), creation_epoch(creation_epoch_), insertion(insertion_) {}
      Node(TYPE &&value_, const bool insertion_, const int64_t creation_epoch_) : value(std::move(value_)), instance_count(insertion_ ? 1 : -1), creation_epoch(creation_epoch_), insertion(insertion_) {}
      Node(Node * const &next_, const TYPE &value_, const bool insertion_, const int64_t creation_epoch_) : next(next_), value(value_), instance_count(insertion_ ? 1 : -1), creation_epoch(creation_epoch_), insertion(insertion_) {}
      Node(Node * const &next_, TYPE &&value_, const bool insertion_, const int64_t creation_epoch_) : next(next_), value(std::move(value_)), instance_count(insertion_ ? 1 : -1), creation_epoch(creation_epoch_), insertion(insertion_) {}
      Node(Node * &&next_, const TYPE &value_, const bool insertion_, const int64_t creation_epoch_) : next(std::move(next_)), value(value_), instance_count(insertion_ ? 1 : -1), creation_epoch(creation_epoch_), insertion(insertion_) {}
      Node(Node * &&next_, TYPE &&value_, const bool insertion_, const int64_t creation_epoch_) : next(std::move(next_)), value(std::move(value_)), instance_count(insertion_ ? 1 : -1), creation_epoch(creation_epoch_), insertion(insertion_) {}

      Node * next = nullptr;
      TYPE value;
      int64_t instance_count = 1;
      int64_t creation_epoch = 0;
      int64_t access_epoch = creation_epoch;
      bool insertion = false;
    };

    struct Cursor {
      Cursor() = default;
      Cursor(Antiable_List * const antiable_list) : raw_cur(antiable_list->m_head), raw_next(masked_cur ? masked_cur->next : nullptr) {}

      bool is_candidate_for_removal(const int64_t earliest_epoch) const {
        const int64_t creation_epoch = masked_cur->creation_epoch;
        const int64_t deletion_epoch = masked_cur->access_epoch;
        if (!(deletion_epoch & 0x1))
          return false;
        return deletion_epoch - earliest_epoch < deletion_epoch - creation_epoch;
      }

      bool is_marked_for_deletion() const {
        return masked_cur && masked_cur->instance_count == 0;
      }

      bool is_end() const {
        return !masked_cur;
      }

      bool increment() {
        if (is_end())
          return false;
        prev = masked_cur;
        masked_cur = masked_next;
        masked_next = masked_cur ? masked_cur->next : nullptr;
        return true;
      }

      Node * prev = nullptr;
      Node * masked_cur = nullptr;
      Node * masked_next = nullptr;
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

      const_iterator(std::mutex &mutex, const int64_t current_epoch)
        : m_mutex_ptr(&mutex),
        m_current_epoch(current_epoch)
      {
      }

      const_iterator(std::mutex &mutex, const int64_t current_epoch, Node * const node)
        : m_mutex_ptr(&mutex),
        m_current_epoch(current_epoch),
        m_node(node)
      {
        validate();
      }

      reference operator*() const {
        std::lock_guard<std::mutex> lock(*m_mutex_ptr);
        return m_node->value;
      }

      const_iterator next() const {
        return ++const_iterator(*this);
      }

      const_iterator & operator++() {
        std::lock_guard<std::mutex> lock(*m_mutex_ptr);
        m_node = m_node->next;
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
            m_node = m_node->next;
            continue;
          }
          const int64_t creation_epoch = m_node->creation_epoch;
          int64_t access_epoch = m_node->access_epoch;
          if (access_epoch - creation_epoch < access_epoch - m_current_epoch || ((access_epoch & 0x1) && access_epoch - creation_epoch < m_current_epoch - creation_epoch)) {
            m_node = reinterpret_cast<Node *>(uintptr_t(m_node->next.load(std::memory_order_relaxed)) & ~uintptr_t(0x1));
            continue;
          }
          m_node->access_epoch = m_current_epoch;
          break;
        }
      }

      int64_t m_current_epoch = 0;
      Node * m_node = nullptr;
      std::mutex * m_mutex_ptr;
    };

    const_iterator cbegin(const int64_t current_epoch) const {
      std::lock_guard<std::mutex> lock(m_mutex);
      return const_iterator(current_epoch, m_head);
    }

    const_iterator cend(const int64_t current_epoch) const {
      std::lock_guard<std::mutex> lock(m_mutex);
      return const_iterator(current_epoch);
    }

    const_iterator begin(const int64_t current_epoch) const {
      std::lock_guard<std::mutex> lock(m_mutex);
      return cbegin(current_epoch);
    }

    const_iterator end(const int64_t current_epoch) const {
      std::lock_guard<std::mutex> lock(m_mutex);
      return cend(current_epoch);
    }

    Antiable_List() noexcept = default;

    ~Antiable_List() noexcept {
      while (m_head) {
        Node * const next = m_head;
        delete m_head;
        m_head = next;
      }
    }

    bool empty() const {
      std::lock_guard<std::mutex> lock(m_mutex);
      return m_head == nullptr;
    }

    int64_t size() const {
      std::lock_guard<std::mutex> lock(m_mutex);
      return m_size;
    }

    int64_t usage() const {
      std::lock_guard<std::mutex> lock(m_mutex);
      return m_usage;
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
      std::lock_guard<std::mutex> lock(m_mutex);

      int64_t earliest_epoch, current_epoch;
      {
        const auto &[earliest_epoch_, current_epoch_] = epoch_list->front_and_acquire();
        earliest_epoch = earliest_epoch_;
        current_epoch = current_epoch_;
      }

      Cursor cursor(this);
      while (cursor.masked_cur && cursor.masked_cur->value < value)
        cursor.increment();
      if (cursor.masked_cur && cursor.masked_cur->value == value) {
        const int64_t instance_count_increment = mode == Mode::Insert ? 1 : -1;
        m_size += instance_count_increment;
        node->instance_count += instance_count_increment;
        if (m_size == 0)
          --m_usage;
        epoch_list->try_release(current_epoch);
        return mode == Mode::Erase && instance_count == 0;
      }

      Node * const node = new Node()
    }

    // Return true if cur removed, otherwise false
    bool try_removal(const std::shared_ptr<Epoch_List> epoch_list, int64_t earliest_epoch, int64_t current_epoch, Cursor &cursor) {
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

      Reclamation_Stacks::push(old_cur->value_ptr);
      Reclamation_Stacks::push(old_cur);

      return true;
    }

    Node * m_head = nullptr;
    int64_t m_size = 0;
    int64_t m_usage = 0;
    mutable std::mutex m_mutex;
  };

}

#endif
