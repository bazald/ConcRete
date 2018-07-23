#ifndef ZENI_CONCURRENCY_EPOCH_LIST_HPP
#define ZENI_CONCURRENCY_EPOCH_LIST_HPP

#include "Shared_Ptr.hpp"

namespace Zeni::Concurrency {

  class Epoch_List {
    Epoch_List(const Epoch_List &) = delete;
    Epoch_List & operator=(const Epoch_List &) = delete;

  public:
    class Token {
      Token(Token &) = delete;
      Token operator=(Token &) = delete;

      friend class Epoch_List;

    protected:
      Token() = default;

    public:
      uint64_t epoch() const {
        std::atomic_thread_fence(std::memory_order_acquire);
        return m_epoch.load();
      }

      bool instantaneous() const {
        return m_instantaneous.load();
      }

    private:
      ZENI_CONCURRENCY_CACHE_ALIGN std::atomic_uint64_t m_epoch = 0;
      ZENI_CONCURRENCY_CACHE_ALIGN std::atomic_bool m_instantaneous = false;
    };

    typedef Shared_Ptr<Token> Token_Ptr;

    static Token_Ptr Create_Token() {
      class Friendly_Token : public Token {};

      return Token_Ptr(new Friendly_Token);
    }

  private:
    struct Node : public Reclamation_Stack::Node {
      Node() = default;
      Node(const int64_t epoch_) : ne(epoch_) {}

      struct ZENI_CONCURRENCY_CACHE_ALIGN_TOGETHER Ne {
        Ne() = default;
        Ne(const int64_t epoch_) : epoch(epoch_) {}

        std::atomic<Node *> next = nullptr;
        uint64_t epoch = 0;
      } ne;
      Token_Ptr token_ptr;
    };

    struct Cursor {
      Cursor() = default;
      Cursor(Epoch_List * const epoch_list) : raw_cur(epoch_list->m_head.load()), raw_next(masked_cur->ne.next.load()) {}

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
        raw_next = masked_cur->ne.next.load();
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
      Node * head = m_head.load();
      while (head) {
        Node * next = reinterpret_cast<Node *>(uintptr_t(head->ne.next.load()) & ~uintptr_t(0x1));
        delete head;
        head = next;
      }
    }

    bool empty() const {
      return m_head.load() == m_tail.load();
    }

    //int64_t size() const {
    //  return m_assigned_epochs.size();
    //}

    uint64_t front() {
      //m_size.fetch_add(1);

      std::atomic_thread_fence(std::memory_order_acquire);

      Cursor cursor(this);
      while (try_removal(cursor));
      const uint64_t front = cursor.masked_cur->ne.epoch;

      return front;
    }

    uint64_t front_and_acquire(const Token_Ptr &current_epoch) {
      //m_size.fetch_add(1);

      //current_epoch.load()->m_instantaneous.store(false);
      const Token_Ptr::Lock current_local = current_epoch.load();
      continue_acquire(current_local);

      std::atomic_thread_fence(std::memory_order_acquire);

      Cursor cursor(this);
      while (try_removal(cursor));
      const uint64_t front = cursor.masked_cur->ne.epoch;

      return front;
    }

    void acquire(const Token_Ptr &current_epoch) {
      //m_size.fetch_add(1);

      //current_epoch.load()->m_instantaneous.store(false);
      const Token_Ptr::Lock current_local = current_epoch.load();
      continue_acquire(current_local);
    }

    void acquire_release(const Token_Ptr &current_epoch) {
      //m_size.fetch_add(1);

      const Token_Ptr::Lock current_local = current_epoch.load();
      current_local->m_instantaneous.store(true);
      continue_acquire(current_local);
    }

    bool try_release(const Token_Ptr::Lock &epoch) {
      //continue_acquire(epoch);
      const bool success = try_erase(epoch->epoch());
      return success;
    }

  private:
    void continue_acquire(const Token_Ptr::Lock &epoch_local) {
      Node * new_tail = nullptr;
      Node * old_tail = m_tail.load();
      std::atomic_thread_fence(std::memory_order_acquire);

      while (!epoch_local->epoch()) {
        Token_Ptr::Lock empty;
        for (;;) {
          if (old_tail->token_ptr.compare_exchange_strong(empty, epoch_local)) {
            update_tail(old_tail, new_tail, epoch_local);
            delete new_tail;
            return;
          }
          else if (update_tail(old_tail, new_tail, empty))
            break;
        }
      }

      delete new_tail;
    }

    bool update_tail(Node * &old_tail, Node * &new_tail, const Token_Ptr::Lock &epoch_current) {
      {
        uint64_t empty = 0;
        if (!epoch_current->m_epoch.compare_exchange_strong(empty, old_tail->ne.epoch) && empty != old_tail->ne.epoch)
          return false;
      }

      if (new_tail)
        new_tail->ne.epoch = old_tail->ne.epoch + epoch_increment;
      else
        new_tail = new Node(old_tail->ne.epoch + epoch_increment);
      std::atomic_thread_fence(std::memory_order_release);
        
      Node * empty = nullptr;
      if (old_tail->ne.next.compare_exchange_strong(empty, epoch_current->instantaneous() ? reinterpret_cast<Node *>(uintptr_t(new_tail) | 0x1) : new_tail)) {
        if (m_tail.compare_exchange_strong(old_tail, new_tail))
          old_tail = new_tail;
        new_tail = nullptr;
      }
      else
        old_tail = m_tail.load();
      std::atomic_thread_fence(std::memory_order_acquire);

      return true;
    }

    bool try_erase(const uint64_t epoch) {
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

          if (cursor.masked_cur->ne.epoch != epoch) {
            cursor.increment();
            continue;
          }

          Node * const marked_next = reinterpret_cast<Node *>(uintptr_t(cursor.raw_next) | 0x1);
          if (cursor.masked_cur->ne.next.compare_exchange_strong(cursor.masked_next, marked_next)) {
            cursor.raw_next = marked_next;
            try_removal(cursor);
            //m_size.fetch_sub(1);
            return true;
          }
          else {
            retry = true;
            break;
          }
        }
      } while (retry);
      return false;
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
      if (!(cursor.prev ? cursor.prev->ne.next : m_head).compare_exchange_strong(cursor.masked_cur, cursor.masked_next)) {
        cursor.masked_cur = old_cur;
        cursor.increment();
        return false;
      }

      cursor.raw_cur = cursor.raw_next;
      cursor.masked_cur = cursor.masked_next;
      if (cursor.masked_cur) {
        cursor.raw_next = cursor.masked_cur->ne.next.load();
        cursor.masked_next = reinterpret_cast<Node *>(uintptr_t(cursor.raw_next) & ~uintptr_t(0x1));
      }
      else {
        cursor.raw_next = nullptr;
        cursor.masked_next = nullptr;
      }

      Reclamation_Stacks::push(old_cur);

      return true;
    }

    ZENI_CONCURRENCY_CACHE_ALIGN std::atomic<Node *> m_head = new Node(1);
    ZENI_CONCURRENCY_CACHE_ALIGN std::atomic<Node *> m_tail = m_head.load();
    //ZENI_CONCURRENCY_CACHE_ALIGN std::atomic_int64_t m_size = 0;
  };
  
}

#endif
