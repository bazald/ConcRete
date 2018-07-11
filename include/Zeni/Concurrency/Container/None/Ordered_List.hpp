#ifndef ZENI_CONCURRENCY_ORDERED_LIST_HPP
#define ZENI_CONCURRENCY_ORDERED_LIST_HPP

namespace Zeni::Concurrency {

  template <typename TYPE>
  class Ordered_List {
    Ordered_List(const Ordered_List &) = delete;
    Ordered_List & operator=(const Ordered_List &) = delete;

    struct Node {
      Node() = default;
      Node(const TYPE &value_) : value(value_) {}
      Node(TYPE &&value_) : value(std::move(value_)) {}
      Node(Node * const &next_, const TYPE &value_) : next(next_), value(value_) {}
      Node(Node * const &next_, TYPE &&value_) : next(next_), value(std::move(value_)) {}
      Node(Node * &&next_, const TYPE &value_) : next(std::move(next_)), value(value_) {}
      Node(Node * &&next_, TYPE &&value_) : next(std::move(next_)), value(std::move(value_)) {}

      Node * next = nullptr;
      TYPE value;
    };

  public:
    Ordered_List() noexcept = default;

    ~Ordered_List() noexcept {
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

    bool front(TYPE &value) {
      if (!m_head)
        return false;
      value = m_head->value;
      return true;
    }

    void insert(const TYPE &value) {
      Node * prev = nullptr;
      Node * node = m_head;
      while (node && node->value < value) {
        prev = node;
        node = node->next;
      }
      (prev ? prev->next : m_head) = new Node(node, value);
    }

    bool try_erase(const TYPE &value) {
      Node * prev = nullptr;
      Node * node = m_head;
      while (node && node->value < value) {
        prev = node;
        node = node->next;
      }
      if (!node || node->value > value)
        return false;
      (prev ? prev->next : m_head) = node->next;
      delete node;
      return true;
    }

  private:
    Node * m_head = nullptr;
    //int64_t m_size = 0;
  };

}

#endif
