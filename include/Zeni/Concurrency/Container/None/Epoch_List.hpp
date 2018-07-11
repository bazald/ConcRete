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
        return m_epoch;
      }

    private:
      uint64_t m_epoch = 0;
    };

    typedef Shared_Ptr<Token> Token_Ptr;

    static Token_Ptr Create_Token() {
      class Friendly_Token : public Token {};

      return Token_Ptr(new Friendly_Token);
    }

  private:
    struct Node {
      Node() = default;
      Node(const int64_t epoch_) : epoch(epoch_) {}

      Node * next = nullptr;
      uint64_t epoch = 0;
    };

  public:
    static const uint64_t epoch_increment = 2;
    
    Epoch_List() = default;

    ~Epoch_List() noexcept {
      while (m_head) {
        Node * next = m_head->next;
        delete m_head;
        m_head = next;
      }
    }

    bool empty() const {
      return m_head == m_tail;
    }

    //int64_t size() const {
    //  return m_size;
    //}

    uint64_t front() {
      return m_head->epoch;
    }

    uint64_t front_and_acquire(const Token_Ptr &current_epoch) {
      {
        if (current_epoch->m_epoch != 0)
          return m_head->epoch;
        current_epoch->m_epoch = m_tail->epoch;
      }
      Node * const tail = new Node(m_tail->epoch + epoch_increment);
      m_tail->next = tail;
      m_tail = tail;
      return m_head->epoch;
    }

    void acquire(const Token_Ptr &current_epoch) {
      {
        if (current_epoch->m_epoch != 0)
          return;
        current_epoch->m_epoch = m_tail->epoch;
      }
      Node * const tail = new Node(m_tail->epoch + epoch_increment);
      m_tail->next = tail;
      m_tail = tail;
    }

    void acquire_release(const Token_Ptr &current_epoch) {
      {
        if (current_epoch->m_epoch != 0)
          return;
        current_epoch->m_epoch = m_tail->epoch;
      }
      m_tail->epoch += epoch_increment;
    }

    bool try_release(const Token_Ptr::Lock &current_epoch) {
      Node * prev = nullptr;
      Node * node = m_head;
      while (node->next && node->epoch != current_epoch->m_epoch) {
        prev = node;
        node = node->next;
      }
      if (!node->next)
        return false;
      (prev ? prev->next : m_head) = node->next;
      delete node;
      return true;
    }

  private:
    Node * m_head = new Node;
    Node * m_tail = m_head;
    //int64_t m_size = 0;
  };

}

#endif
