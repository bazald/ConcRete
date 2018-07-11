#ifndef ZENI_CONCURRENCY_ANTIABLE_LIST_HPP
#define ZENI_CONCURRENCY_ANTIABLE_LIST_HPP

#include "Epoch_List.hpp"

#include <cassert>

namespace Zeni::Concurrency {

  template <typename TYPE>
  class Antiable_List {
    Antiable_List(const Antiable_List &) = delete;
    Antiable_List & operator=(const Antiable_List &) = delete;

    struct Node {
      Node() = default;
      Node(const TYPE &value_, const bool insertion_) : value(value_), instance_count(insertion_ ? 1 : -1), insertion(insertion_) {}
      Node(TYPE &&value_, const bool insertion_) : value(std::move(value_)), instance_count(insertion_ ? 1 : -1), insertion(insertion_) {}
      Node(Node * const &next_, const TYPE &value_, const bool insertion_) : next(next_), value(value_), instance_count(insertion_ ? 1 : -1), insertion(insertion_) {}
      Node(Node * const &next_, TYPE &&value_, const bool insertion_) : next(next_), value(std::move(value_)), instance_count(insertion_ ? 1 : -1), insertion(insertion_) {}
      Node(Node * &&next_, const TYPE &value_, const bool insertion_) : next(std::move(next_)), value(value_), instance_count(insertion_ ? 1 : -1), insertion(insertion_) {}
      Node(Node * &&next_, TYPE &&value_, const bool insertion_) : next(std::move(next_)), value(std::move(value_)), instance_count(insertion_ ? 1 : -1), insertion(insertion_) {}

      Node * next = nullptr;
      TYPE value;
      uint64_t instance_count = 1;
      Epoch_List::Token_Ptr creation_epoch = Zeni::Concurrency::Epoch_List::Create_Token();
      Epoch_List::Token_Ptr deletion_epoch = Zeni::Concurrency::Epoch_List::Create_Token();
      bool insertion = false;
    };

    struct Cursor {
      Cursor() = default;
      Cursor(Antiable_List * const antiable_list) : masked_cur(antiable_list->m_head), masked_next(masked_cur ? masked_cur->next : nullptr) {}

      bool is_candidate_for_removal(const int64_t earliest_epoch) const {
        const uint64_t deletion_epoch = masked_cur->deletion_epoch;
        if (deletion_epoch == 0)
          return false;
        const uint64_t creation_epoch = masked_cur->creation_epoch;
        return deletion_epoch - creation_epoch < earliest_epoch - creation_epoch;
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

      const_iterator(const uint64_t earliest_epoch, const uint64_t current_epoch)
        : m_earliest_epoch(earliest_epoch),
        m_current_epoch(current_epoch)
      {
      }

      const_iterator(const uint64_t earliest_epoch, const uint64_t current_epoch, Node * const node)
        : m_earliest_epoch(earliest_epoch),
        m_current_epoch(current_epoch),
        m_node(node)
      {
        validate();
      }

      reference operator*() const {
        return m_node->value;
      }

      const_iterator next() const {
        return ++const_iterator(*this);
      }

      const_iterator & operator++() {
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
          const uint64_t creation_epoch = m_node->creation_epoch->epoch();
          const uint64_t deletion_epoch = m_node->deletion_epoch->epoch();
          if (deletion_epoch ? deletion_epoch - creation_epoch < m_current_epoch - creation_epoch : creation_epoch ? creation_epoch - m_earliest_epoch > m_current_epoch - m_earliest_epoch : true) {
            m_node = m_node->next;
            continue;
          }
          break;
        }
      }

      uint64_t m_earliest_epoch = 0;
      uint64_t m_current_epoch = 0;
      Node * m_node = nullptr;
    };

    const_iterator cbegin(const uint64_t earliest_epoch, const uint64_t current_epoch) const {
      return const_iterator(earliest_epoch, current_epoch, m_head);
    }

    const_iterator cend(const uint64_t earliest_epoch, const uint64_t current_epoch) const {
      return const_iterator(earliest_epoch, current_epoch);
    }

    const_iterator begin(const uint64_t earliest_epoch, const uint64_t current_epoch) const {
      return cbegin(earliest_epoch, current_epoch);
    }

    const_iterator end(const uint64_t earliest_epoch, const uint64_t current_epoch) const {
      return cend(earliest_epoch, current_epoch);
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
      return m_head == nullptr;
    }

    int64_t size() const {
      return m_size;
    }

    int64_t usage() const {
      return m_usage;
    }

    /// Return true if it decrements the count to 0, otherwise false
    bool erase(const std::shared_ptr<Epoch_List> epoch_list, const TYPE &value) {
      return access(epoch_list, value, nullptr, Mode::Erase) == 0;
    }

    /// Return true if it decrements the count to 0, otherwise false
    bool erase(const std::shared_ptr<Epoch_List> epoch_list, const TYPE &value, Epoch_List::Token_Ptr::Lock &erasure_epoch) {
      return access(epoch_list, value, &erasure_epoch, Mode::Erase) == 0;
    }

    /// Return true if it increments the count to 1, otherwise false
    bool insert(const std::shared_ptr<Epoch_List> epoch_list, const TYPE &value) {
      return access(epoch_list, value, nullptr, Mode::Insert) == 1;
    }

    /// Return true if it increments the count to 1, otherwise false
    bool insert(const std::shared_ptr<Epoch_List> epoch_list, const TYPE &value, Epoch_List::Token_Ptr::Lock &insertion_epoch) {
      return access(epoch_list, value, &insertion_epoch, Mode::Insert) == 1;
    }

  private:
    /// Return true if it increments the count to 1, otherwise false
    int64_t access(const std::shared_ptr<Epoch_List> epoch_list, const TYPE &value, Epoch_List::Token_Ptr::Lock * const epoch, const Mode mode) {
      const uint64_t earliest_epoch = epoch_list->front();

      Cursor cursor(this);
      while (cursor.masked_cur && cursor.masked_cur->value < value)
        cursor.increment();
      if (cursor.masked_cur && cursor.masked_cur->value == value) {
        const int64_t instance_count_increment = mode == Mode::Insert ? 1 : -1;
        if (cursor.masked_cur->insertion)
          m_size += instance_count_increment;
        cursor.masked_cur->instance_count += instance_count_increment;
        if (cursor.masked_cur->instance_count == 0) {
          --m_usage;
          if (mode == Mode::Erase && epoch) {
            epoch_list->acquire(cursor.masked_cur->deletion_epoch);
            *epoch = cursor.masked_cur->deletion_epoch.load();
            assert(*epoch);
          }
          else
            epoch_list->acquire_release(cursor.masked_cur->deletion_epoch);
          (cursor.prev ? cursor.prev->next : m_head) = cursor.masked_next;
          delete cursor.masked_cur;
          return 0;
        }
        else
          return cursor.masked_cur->instance_count;
      }

      Node * const new_value = new Node(cursor.prev ? cursor.masked_cur : m_head, value, mode == Mode::Insert);
      (cursor.prev ? cursor.prev->next : m_head) = new_value;
      if (mode == Mode::Insert)
        ++m_size;
      ++m_usage;
      if (mode == Mode::Insert && epoch) {
        epoch_list->acquire(new_value->creation_epoch);
        *epoch = new_value->creation_epoch.load();
        assert(*epoch);
      }
      else
        epoch_list->acquire_release(new_value->creation_epoch);
      return mode == Mode::Insert ? 1 : -1;
    }

    Node * m_head = nullptr;
    int64_t m_size = 0;
    int64_t m_usage = 0;
  };

}

#endif
