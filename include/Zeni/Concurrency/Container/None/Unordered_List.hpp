#ifndef ZENI_CONCURRENCY_UNORDERED_LIST_HPP
#define ZENI_CONCURRENCY_UNORDERED_LIST_HPP

namespace Zeni::Concurrency {

  template <typename TYPE>
  class Unordered_List {
    Unordered_List(const Unordered_List &) = delete;
    Unordered_List & operator=(const Unordered_List &) = delete;

    struct Node {
      Node() = default;
      Node(const TYPE &value_) : value(value_) {}
      Node(TYPE &&value_) : value(std::move(value)) {}
      Node(Node * const &next_, const TYPE &value_) : next(next_), value(value_) {}
      Node(Node * const &next_, TYPE &&value_) : next(next_), value(std::move(value)) {}
      Node(Node * &&next_, const TYPE &value_) : next(std::move(next_)), value(value_) {}
      Node(Node * &&next_, TYPE &&value_) : next(std::move(next_)), value(std::move(value)) {}

      Node * next = nullptr;
      TYPE value;
    };

  public:
    Unordered_List() noexcept = default;

    ~Unordered_List() noexcept {
      while (m_head) {
        Node * const next = m_head->next;
        delete m_head;
        m_head = next;
      }
    }

    bool empty() const {
      return m_head == nullptr;
    }

    //int64_t size() const {
    //  return m_size.load(std::memory_order_relaxed);
    //}

    void push_front(const TYPE &value) {
      //++m_size;
      m_head = new Node(m_head, value);
      if (!m_tail)
        m_tail = m_head;
    }

    void push_front(TYPE &&value) {
      //++m_size;
      m_head = new Node(m_head, std::move(value));
      if (!m_tail)
        m_tail = m_head;
    }

    void push_back(const TYPE &value) {
      //++m_size;
      Node * const tail = new Node(value);
      (m_tail ? m_tail->next : m_head) = tail;
      m_tail = tail;
    }

    void push_back(TYPE &&value) {
      //++m_size;
      Node * const tail = new Node(std::move(value));
      (m_tail ? m_tail->next : m_head) = tail;
      m_tail = tail;
    }

    bool try_erase(const TYPE &value) {
      //--m_size;
      Node * prev = nullptr;
      Node * node = m_head;
      while (node && node->value != value) {
        prev = node;
        node = node->next;
      }
      if (!node)
        return false;
      (prev ? prev->next : m_head) = node->next;
      if (m_tail == node)
        m_tail = prev;
      delete node;
      return true;
    }

  private:
    Node * m_head = nullptr;
    Node * m_tail = nullptr;
    //int64_t m_size = 0;
  };

}

#endif
