#ifndef ZENI_CONCURRENCY_EPOCH_LIST_HPP
#define ZENI_CONCURRENCY_EPOCH_LIST_HPP

#include <mutex>

namespace Zeni::Concurrency {

  class Epoch_List {
    Epoch_List(const Epoch_List &) = delete;
    Epoch_List & operator=(const Epoch_List &) = delete;

    struct Node {
      Node() = default;
      Node(const int64_t epoch_) : epoch(epoch_) {}

      Node * next = nullptr;
      uint64_t epoch = 2;
    };

  public:
    static const uint64_t epoch_increment = 4;
    
    Epoch_List() = default;

    ~Epoch_List() noexcept {
      while (m_head) {
        Node * next = m_head->next;
        delete m_head;
        m_head = next;
      }
    }

    bool empty() const {
      std::lock_guard<std::mutex> lock(m_mutex);
      return m_head == m_tail;
    }

    std::pair<uint64_t, uint64_t> front_and_acquire() {
      std::lock_guard<std::mutex> lock(m_mutex);
      const uint64_t acquired = m_tail->epoch;
      Node * const tail = new Node(m_tail->epoch + epoch_increment);
      m_tail->next = tail;
      m_tail = tail;
      return std::make_pair(m_head->epoch, acquired);
    }

    //int64_t size() const {
    //  return m_size;
    //}

    uint64_t acquire() {
      std::lock_guard<std::mutex> lock(m_mutex);
      const uint64_t acquired = m_tail->epoch;
      Node * const tail = new Node(m_tail->epoch + epoch_increment);
      m_tail->next = tail;
      m_tail = tail;
      return acquired;
    }

    bool try_release(const uint64_t epoch) {
      std::lock_guard<std::mutex> lock(m_mutex);
      Node * prev = nullptr;
      Node * node = m_head;
      while (node->next && node->epoch < epoch) {
        prev = node;
        node = node->next;
      }
      if (!node->next || node->epoch > epoch)
        return false;
      (prev ? prev->next : m_head) = node->next;
      delete node;
      return true;
    }

  private:
    Node * m_head = new Node;
    Node * m_tail = m_head;
    //int64_t m_size = 0;
    mutable std::mutex m_mutex;
  };

}

#endif
