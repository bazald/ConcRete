#ifndef ZENI_CONCURRENCY_QUEUE_HPP
#define ZENI_CONCURRENCY_QUEUE_HPP

#include <mutex>

namespace Zeni::Concurrency {

  template <typename TYPE>
  class Queue {
    Queue(const Queue &) = delete;
    Queue & operator=(const Queue &) = delete;

    struct Node {
      Node(const TYPE &value_) : value(value_) {}
      Node(TYPE &&value_) : value(std::move(value_)) {}

      Node * next = nullptr;
      TYPE value;
    };

  public:
    Queue() noexcept {
    }

    ~Queue() noexcept {
      while (m_head) {
        Node * const next = m_head;
        delete m_head;
        m_head = next;
      }
    }

    bool empty() const {
      return m_head == nullptr;
    }

    //int64_t size() const {
    //  std::lock_guard lock(m_mutex);
    //  return m_size;
    //}

    void push(const TYPE &value) {
      Node * const tail = new Node(value);
      //++m_size;
      if (m_tail)
        m_tail->next = tail;
      m_tail = tail;
      if (!m_head)
        m_head = m_tail;
    }

    void push(TYPE &&value) {
      Node * const tail = new Node(std::move(value));
      //++m_size;
      if (m_tail)
        m_tail->next = tail;
      m_tail = tail;
      if (!m_head)
        m_head = m_tail;
    }

    bool try_pop(TYPE &value) {
      Node * head;
      {
        if (!m_head)
          return false;
        //--m_size;
        head = m_head;
        m_head = m_head->next;
        if (!m_head)
          m_tail = nullptr;
      }
      value = std::move(head->value);
      delete head;
      return true;
    }

  private:
    Node * m_head = nullptr;
    Node * m_tail = nullptr;
    //int64_t m_size = 0;
  };

}

#endif
