#ifndef ZENI_CONCURRENCY_STACK_HPP
#define ZENI_CONCURRENCY_STACK_HPP

#include <mutex>

namespace Zeni::Concurrency {

  template <typename TYPE>
  class Stack {
    Stack(const Stack &) = delete;
    Stack & operator=(const Stack &) = delete;

    struct Node {
      Node() = default;
      Node(Node * const &next_, const TYPE &value_) : next(next_), value(value_) {}
      Node(Node * const &next_, TYPE &&value_) : next(next_), value(std::move(value_)) {}
      Node(Node * &&next_, const TYPE &value_) : next(std::move(next_)), value(value_) {}
      Node(Node * &&next_, TYPE &&value_) : next(std::move(next_)), value(std::move(value_)) {}

      Node * next = nullptr;
      TYPE value;
    };

  public:
    Stack() noexcept = default;

    ~Stack() noexcept {
      while (m_head) {
        Node * const next = m_head->next;
        delete m_head;
        m_head = next;
      }
    }

    bool empty() const {
      std::lock_guard<std::mutex> lock(m_mutex);
      return m_head == nullptr;
    }

    //int64_t size() const {
    //  std::lock_guard<std::mutex> lock(m_mutex);
    //  return m_size;
    //}

    void push(const TYPE &value) {
      std::lock_guard<std::mutex> lock(m_mutex);
      //++m_size;
      m_head = new Node(m_head, value);
    }

    void push(TYPE &&value) {
      std::lock_guard<std::mutex> lock(m_mutex);
      //++m_size;
      m_head = new Node(m_head, std::move(value));
    }

    bool try_pop(TYPE &value) {
      std::lock_guard<std::mutex> lock(m_mutex);
      if (!m_head)
        return false;
      //--m_size;
      Node * const next = m_head->next;
      value = std::move(m_head->value);
      delete m_head;
      m_head = next;
      return true;
    }

  private:
    Node * m_head = nullptr;
    //int64_t m_size = 0;
    mutable std::mutex m_mutex;
  };

}

#endif
