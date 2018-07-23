#ifndef ZENI_CONCURRENCY_ANTIABLE_HASHSET_HPP
#define ZENI_CONCURRENCY_ANTIABLE_HASHSET_HPP

#include "Epoch_List.hpp"

#include "../../Internal/Reclamation_Stacks.hpp"

namespace Zeni::Concurrency {

  template <typename TYPE, typename HASH = std::hash<TYPE>>
  class Antiable_Hashset {
  public:
    Antiable_Hashset(const Antiable_Hashset &) = delete;
    Antiable_Hashset & operator=(const Antiable_Hashset &) = delete;

    struct Node : public Reclamation_Stack::Node {
      Node() = default;
      Node(const TYPE &value_, const bool insertion_) : cat(insertion_), value(value_) {}
      Node(Node * const &next_in_bucket_, const TYPE &value_, const bool insertion_) : next_in_bucket(next_in_bucket_), cat(insertion_), value(value_) {}
      Node(Node * &&next_in_bucket_, const TYPE &value_, const bool insertion_) : next_in_bucket(std::move(next_in_bucket_)), cat(insertion_), value(value_) {}

      ZENI_CONCURRENCY_CACHE_ALIGN std::atomic<Node *> next_in_list = nullptr;
      ZENI_CONCURRENCY_CACHE_ALIGN std::atomic<Node *> next_in_bucket = nullptr;
      struct ZENI_CONCURRENCY_CACHE_ALIGN_TOGETHER CAT {
        CAT(const bool insertion_) : instance_count(insertion_ ? 1 : -1), insertion(insertion_) {}
        std::atomic<int64_t> instance_count;
        bool insertion;
      } cat;
      Epoch_List::Token_Ptr creation_epoch = Zeni::Concurrency::Epoch_List::Create_Token();
      Epoch_List::Token_Ptr deletion_epoch = Zeni::Concurrency::Epoch_List::Create_Token();
      TYPE value;
    };

    struct Bucket {
      ZENI_CONCURRENCY_CACHE_ALIGN std::atomic<Node *> bucket_head = nullptr;
    };

    struct Cursor {
      Cursor() = default;
      Cursor(Bucket &bucket) : bucket_head(&bucket.bucket_head), raw_cur(bucket_head->load()), raw_next(masked_cur ? masked_cur->next_in_bucket.load() : nullptr) {}

      bool is_candidate_for_removal(const uint64_t earliest_epoch) const {
        if (raw_cur != masked_cur || raw_next == masked_next)
          return false;
        const uint64_t deletion_epoch = masked_cur->deletion_epoch.load()->epoch();
        return deletion_epoch && earliest_epoch > deletion_epoch;
      }

      bool is_marked_for_deletion() const {
        return masked_cur && masked_cur->cat.instance_count.load() == 0;
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
          raw_next = masked_cur->next_in_bucket.load();
          masked_next = reinterpret_cast<Node *>(uintptr_t(raw_next) & ~uintptr_t(0x1));
        }
        else {
          raw_next = nullptr;
          masked_next = nullptr;
        }
        return true;
      }

      std::atomic<Node *> * bucket_head = nullptr;
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

      const_iterator(const uint64_t current_epoch, Node * const node)
        : m_current_epoch(current_epoch),
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
        m_node = reinterpret_cast<Node *>(uintptr_t(m_node->next_in_list.load()) & ~uintptr_t(0x1));
        validate();
        return *this;
      }
      const_iterator operator++(int) {
        const_iterator rv(*this);
        ++*this;
        return rv;
      }

      bool operator==(const const_iterator &rhs) const {
        return m_node == rhs.m_node;
      }
      bool operator!=(const const_iterator &rhs) const {
        return m_node != rhs.m_node;
      }

      uint64_t creation_epoch() const {
        return m_node->creation_epoch.load()->epoch();
      }

      uint64_t deletion_epoch() const {
        return m_node->deletion_epoch.load()->epoch();
      }

    //private:
      void validate() {
        for (;;) {
          if (!m_node)
            break;
          if (!m_node->cat.insertion) {
            m_node = reinterpret_cast<Node *>(uintptr_t(m_node->next_in_list.load()) & ~uintptr_t(0x1));
            continue;
          }
          const uint64_t deletion_epoch = m_node->deletion_epoch.load()->epoch();
          const uint64_t creation_epoch = m_node->creation_epoch.load()->epoch();
          assert(!deletion_epoch || creation_epoch);
          if (creation_epoch && creation_epoch < m_current_epoch && (!deletion_epoch || deletion_epoch > m_current_epoch)) {
            std::atomic_thread_fence(std::memory_order_acquire);
            break;
          }
          else {
            m_node = reinterpret_cast<Node *>(uintptr_t(m_node->next_in_list.load()) & ~uintptr_t(0x1));
            continue;
          }
        }
      }

      uint64_t m_current_epoch = 0;
      Node * m_node = nullptr;
    };

    const_iterator cbegin(const typename Epoch_List::Token_Ptr::Lock &current_epoch) const {
      std::atomic_thread_fence(std::memory_order_acquire);
      return const_iterator(current_epoch->epoch(), m_head.load());
    }

    const_iterator cbegin(const uint64_t current_epoch) const {
      std::atomic_thread_fence(std::memory_order_acquire);
      return const_iterator(current_epoch, m_head.load());
    }

    const_iterator cend() const {
      return const_iterator();
    }

    const_iterator begin(const typename Epoch_List::Token_Ptr::Lock &current_epoch) const {
      return cbegin(current_epoch->epoch());
    }

    const_iterator begin(const uint64_t current_epoch) const {
      return cbegin(current_epoch);
    }

    const_iterator end() const {
      return cend();
    }

    Antiable_Hashset() noexcept {
      std::atomic_thread_fence(std::memory_order_release);
    }

    ~Antiable_Hashset() noexcept {
      std::atomic_thread_fence(std::memory_order_acquire);
      Node * head = m_head.load();
      while (head) {
        Node * const next = reinterpret_cast<Node *>(uintptr_t(head->next_in_list.load()) & ~uintptr_t(0x1));
        delete head;
        head = next;
      }
      delete m_buckets1.load();
    }

    bool empty() const {
      return m_head.load() == nullptr;
    }

    int64_t size() const {
      return m_size.load();
    }

    int64_t usage() const {
      return m_usage.load();
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

  //private:
    /// Return true if it increments the count to 1, otherwise false
    int64_t access(const std::shared_ptr<Epoch_List> epoch_list, const TYPE &value, Epoch_List::Token_Ptr::Lock * const epoch, const Mode mode) {
      const uint64_t earliest_epoch = epoch_list->front();

      Node * new_value = nullptr;
      Bucket &bucket = m_buckets1.load()[HASH()(value) % m_num_buckets1.load()];
      for (;;) {
        Cursor last_unmarked;
        Cursor cursor(bucket);
        Node * head = cursor.raw_cur;

        while (try_removal(epoch_list, earliest_epoch, cursor));

        for (;;) {
          if (cursor.is_marked_for_deletion()) {
            while (try_removal(epoch_list, earliest_epoch, cursor));
            continue;
          }

          if (!cursor.is_end() && (!cursor.masked_cur || cursor.masked_cur->value < value)) {
            last_unmarked = cursor;
            cursor.increment();
            continue;
          }

          if (cursor.masked_cur && cursor.masked_cur->value == value) {
            int64_t instance_count = cursor.masked_cur->cat.instance_count.load();
            const int64_t instance_count_increment = mode == Mode::Insert ? 1 : -1;
            epoch_list->acquire(cursor.masked_cur->creation_epoch);
            while (instance_count) {
              if (cursor.masked_cur->cat.instance_count.compare_exchange_strong(instance_count, instance_count + instance_count_increment)) {
                if (cursor.masked_cur->cat.insertion)
                  m_size.fetch_add(instance_count_increment);
                instance_count += instance_count_increment;
                if (instance_count == 0) {
                  m_usage.fetch_sub(1);
                  epoch_list->acquire(cursor.masked_cur->deletion_epoch);
                  if (mode == Mode::Erase && epoch) {
                    *epoch = cursor.masked_cur->deletion_epoch.load();
                    assert(*epoch);
                  }
                  else
                    epoch_list->try_release(cursor.masked_cur->deletion_epoch.load());
                  try_removal(epoch_list, earliest_epoch, cursor);
                }
                delete new_value;
                return instance_count;
              }
            }
            epoch_list->acquire(cursor.masked_cur->deletion_epoch);
            break;
          }

          if (!new_value) {
            new_value = new Node(value, mode == Mode::Insert);
            std::atomic_thread_fence(std::memory_order_release);
          }

          new_value->next_in_bucket.store(last_unmarked.masked_cur ? last_unmarked.masked_next : head);
          if ((last_unmarked.masked_cur ? last_unmarked.masked_cur->next_in_bucket : *cursor.bucket_head).compare_exchange_strong(last_unmarked.masked_cur ? last_unmarked.masked_next : head, new_value)) {
            if (mode == Mode::Insert)
              m_size.fetch_add(1);
            m_usage.fetch_add(1);

            do {
              new_value->next_in_list = head;
            } while (!m_head.compare_exchange_strong(head, new_value));

            epoch_list->acquire(new_value->creation_epoch);
            if (mode == Mode::Insert && epoch) {
              *epoch = new_value->creation_epoch.load();
              assert(*epoch);
            }
            else
              epoch_list->try_release(new_value->creation_epoch.load());
            return mode == Mode::Insert ? 1 : -1;
          }
          else
            break;
        }
      }
    }

    // Return true if cur removed, otherwise false
    bool try_removal(const std::shared_ptr<Epoch_List> epoch_list, const uint64_t earliest_epoch, Cursor &cursor) {
      if (!cursor.is_marked_for_deletion())
        return false;

      while (!(uintptr_t(cursor.raw_next) & 0x1)) {
        Node * const marked_next = reinterpret_cast<Node *>(uintptr_t(cursor.raw_next) | 0x1);
        if (cursor.masked_cur->next_in_bucket.compare_exchange_strong(cursor.raw_next, marked_next)) {
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
      if (!(cursor.prev ? cursor.prev->next_in_bucket : *cursor.bucket_head).compare_exchange_strong(cursor.masked_cur, cursor.masked_next)) {
        cursor.masked_cur = old_cur;
        cursor.increment();
        return false;
      }

      cursor.raw_cur = cursor.raw_next;
      cursor.masked_cur = cursor.masked_next;
      if (cursor.masked_cur) {
        cursor.raw_next = cursor.masked_cur->next_in_bucket.load();
        cursor.masked_next = reinterpret_cast<Node *>(uintptr_t(cursor.raw_next) & ~uintptr_t(0x1));
      }
      else {
        cursor.raw_next = nullptr;
        cursor.masked_next = nullptr;
      }

      for (;;) {
        Node * prev = nullptr;
        Node * raw_cur = m_head.load();
        Node * masked_cur = reinterpret_cast<Node *>(uintptr_t(raw_cur) & ~uintptr_t(0x1));
        Node * raw_next = masked_cur ? masked_cur->next_in_list.load() : nullptr;
        Node * masked_next = reinterpret_cast<Node *>(uintptr_t(raw_next) & ~uintptr_t(0x1));
        while (masked_cur != old_cur) {
          if (!masked_cur)
            return true;

          if (raw_next != masked_next && raw_cur == masked_cur) {
            if ((prev ? prev->next_in_list : m_head).compare_exchange_strong(masked_cur, masked_next)) {
              Reclamation_Stacks::push(masked_cur);
              raw_cur = raw_next;
              masked_cur = masked_next;
              raw_next = masked_cur ? masked_cur->next_in_list.load() : nullptr;
              masked_next = reinterpret_cast<Node *>(uintptr_t(raw_next) & ~uintptr_t(0x1));
              continue;
            }
            else
              masked_cur = reinterpret_cast<Node *>(uintptr_t(raw_cur) & ~uintptr_t(0x1));
          }

          prev = masked_cur;
          raw_cur = raw_next;
          masked_cur = masked_next;
          raw_next = masked_cur ? masked_cur->next_in_list.load() : nullptr;
          masked_next = reinterpret_cast<Node *>(uintptr_t(raw_next) & ~uintptr_t(0x1));
        }

        if (raw_next == masked_next) {
          for (;;) {
            Node * const marked_next = reinterpret_cast<Node *>(uintptr_t(raw_next) | 0x1);
            if (masked_cur->next_in_list.compare_exchange_strong(raw_next, marked_next)) {
              raw_next = marked_next;
              break;
            }
            else
              masked_next = reinterpret_cast<Node *>(uintptr_t(raw_next) & ~uintptr_t(0x1));
          }
        }

        if (raw_cur == masked_cur) {
          if ((prev ? prev->next_in_list : m_head).compare_exchange_strong(masked_cur, masked_next)) {
            Reclamation_Stacks::push(masked_cur);
            break;
          }
        }
      }

      return true;
    }

    ZENI_CONCURRENCY_CACHE_ALIGN std::atomic<Node *> m_head = nullptr;
    ZENI_CONCURRENCY_CACHE_ALIGN std::atomic_uint64_t m_num_buckets1 = 64;
    ZENI_CONCURRENCY_CACHE_ALIGN std::atomic_uint64_t m_num_buckets2 = 2 * m_num_buckets1.load();
    ZENI_CONCURRENCY_CACHE_ALIGN std::atomic<Bucket *> m_buckets1 = new Bucket[m_num_buckets1];
    ZENI_CONCURRENCY_CACHE_ALIGN std::atomic<Bucket *> m_buckets2 = nullptr;
    ZENI_CONCURRENCY_CACHE_ALIGN std::atomic_int64_t m_size = 0;
    ZENI_CONCURRENCY_CACHE_ALIGN std::atomic_int64_t m_usage = 0;
  };

}

#endif
