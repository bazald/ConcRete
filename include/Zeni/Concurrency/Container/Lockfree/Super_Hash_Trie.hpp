#ifndef ZENI_CONCURRENCY_SUPER_HASH_TRIE_HPP
#define ZENI_CONCURRENCY_SUPER_HASH_TRIE_HPP

#include "Intrusive_Shared_Ptr.hpp"

#include <functional>
#include <tuple>

namespace Zeni::Concurrency {

  namespace Internal {
    template <size_t tuple_size>
    struct Update_Tuple_1 {};

    template <>
    struct Update_Tuple_1<3> {
      template <typename Tuple_Tupe, typename Updated_Node_Type>
      static auto updated(Tuple_Tupe tuple_value, Updated_Node_Type updated_node) {
        return std::make_tuple(std::get<0>(tuple_value), updated_node, std::get<2>(tuple_value));
      }
    };

    template <>
    struct Update_Tuple_1<4> {
      template <typename Tuple_Tupe, typename Updated_Node_Type>
      static auto updated(Tuple_Tupe tuple_value, Updated_Node_Type updated_node) {
        return std::make_tuple(std::get<0>(tuple_value), updated_node, std::get<2>(tuple_value), std::get<3>(tuple_value));
      }
    };
  }

  template <typename... Types>
  class Super_Hash_Trie {
    class Hash_Trie_Super_Node : public Enable_Intrusive_Sharing<Hash_Trie_Super_Node> {
      Hash_Trie_Super_Node & operator=(const Hash_Trie_Super_Node &rhs) = delete;

    public:
      Hash_Trie_Super_Node() = default;

      Hash_Trie_Super_Node(const Hash_Trie_Super_Node &rhs)
        : m_hash_tries(rhs.m_hash_tries)
      {
      }

      Hash_Trie_Super_Node(Types&&... types) {
        initialize(std::forward<Types...>(types...));
      }

      template <size_t index>
      bool empty() const {
        return std::get<index>(m_hash_tries).empty();
      }

      template <size_t index>
      size_t size() const {
        return std::get<index>(m_hash_tries).size();
      }

      template <size_t index>
      bool size_one() const {
        return std::get<index>(m_hash_tries).size_one();
      }

      template <size_t index, typename Key>
      auto looked_up(const Key &key) const {
        return std::get<index>(m_hash_tries).looked_up(key);
      }

      template <size_t index, typename Key>
      auto inserted(const Key &key) const {
        auto tuple_value = std::get<index>(m_hash_tries).inserted(key);
        Hash_Trie_Super_Node * const updated_node = new Hash_Trie_Super_Node(*this);
        std::get<index>(updated_node->m_hash_tries) = std::get<1>(tuple_value);
        return Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, updated_node);
      }

      template <size_t index, typename Key>
      auto erased(const Key &key) const {
        auto tuple_value = std::get<index>(m_hash_tries).erased(key);
        Hash_Trie_Super_Node * const updated_node = new Hash_Trie_Super_Node(*this);
        std::get<index>(updated_node->m_hash_tries) = std::get<1>(tuple_value);
        return Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, updated_node);
      }

      template <size_t index>
      auto snapshot() const {
        return std::get<index>(m_hash_tries).snapshot();
      }

    private:
      void initialize(const size_t) {}

      template <typename First, typename... Rest>
      void initialize(First &&first, Rest&&... rest, const size_t index = 0) {
        std::get<index>(m_hash_tries) = std::forward<First>(first);
        initialize(rest..., index + 1);
      }

      std::tuple<Types ...> m_hash_tries;
    };

  public:
    typedef Super_Hash_Trie<Types...> Snapshot;

    Super_Hash_Trie()
      : m_super_root(new Hash_Trie_Super_Node())
    {
    }

    ~Super_Hash_Trie() {
      const Hash_Trie_Super_Node * super_root = m_super_root.load(std::memory_order_acquire);
      if (super_root)
        super_root->decrement_refs();
    }

    Super_Hash_Trie(const Super_Hash_Trie &rhs)
      : m_super_root(rhs.isnapshot())
    {
    }

    Super_Hash_Trie & operator=(const Super_Hash_Trie &rhs) {
      const Hash_Trie_Super_Node * super_root = m_super_root.load(std::memory_order_acquire);
      const Hash_Trie_Super_Node * const new_super_root = rhs.isnapshot();
      CAS(m_super_root, super_root, new_super_root, std::memory_order_release, std::memory_order_acquire);
      return *this;
    }

    Super_Hash_Trie(Super_Hash_Trie &&rhs)
      : m_super_root(rhs.m_super_root.load(std::memory_order_relaxed))
    {
      rhs.m_super_root.store(nullptr, std::memory_order_relaxed);
    }

    template <size_t index>
    bool empty() const {
      const auto super_root = m_super_root.load(std::memory_order_acquire);
      return super_root->template empty<index>();
    }

    template <size_t index>
    size_t size() const {
      const auto super_root = m_super_root.load(std::memory_order_acquire);
      return super_root->template size<index>();
    }

    template <size_t index>
    bool size_one() const {
      const auto super_root = m_super_root.load(std::memory_order_acquire);
      return super_root->template size_one<index>();
    }

    template <size_t index, typename Key>
    auto lookup(const Key &key) const {
      const Hash_Trie_Super_Node * const super_root = isnapshot();
      const auto found = super_root->template looked_up<index>(key);
      return std::make_pair(found, Snapshot(super_root));
    }

    template <size_t index, typename Key>
    auto insert(const Key &key) {
      return insert<index>(key, true);
    }

    template <size_t index, typename Key>
    auto erase(const Key &key) {
      const auto tuple_value = erase<index>(key, false);
      return std::make_tuple(std::get<0>(tuple_value), std::get<1>(tuple_value), std::get<2>(tuple_value));
    }

    Snapshot snapshot() const {
      return isnapshot();
    }

    template <size_t index>
    auto snapshot() const {
      const Hash_Trie_Super_Node * super_root = m_super_root.load(std::memory_order_acquire);
      return super_root->template snapshot<index>();
    }

  private:
    template <size_t index, typename Key>
    auto insert(const Key &key, const bool insertion) {
      const Hash_Trie_Super_Node * super_root = isnapshot();
      for (;;) {
        const auto tuple_value = super_root->template inserted<index>(key);
        if (std::get<1>(tuple_value))
          std::get<1>(tuple_value)->increment_refs();
        if (super_root)
          super_root->decrement_refs();
        if (CAS(m_super_root, super_root, std::get<1>(tuple_value), std::memory_order_release, std::memory_order_acquire))
          return Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, Snapshot(std::get<1>(tuple_value)));
        else {
          if (std::get<1>(tuple_value))
            std::get<1>(tuple_value)->decrement_refs();
          enforce_snapshot(super_root);
        }
      }
    }

    template <size_t index, typename Key>
    auto erase(const Key &key, const bool insertion) {
      const Hash_Trie_Super_Node * super_root = isnapshot();
      for (;;) {
        const auto tuple_value = super_root->template erased<index>(key);
        if (std::get<1>(tuple_value))
          std::get<1>(tuple_value)->increment_refs();
        if (super_root)
          super_root->decrement_refs();
        if (CAS(m_super_root, super_root, std::get<1>(tuple_value), std::memory_order_release, std::memory_order_acquire))
          return Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, Snapshot(std::get<1>(tuple_value)));
        else {
          if (std::get<1>(tuple_value))
            std::get<1>(tuple_value)->decrement_refs();
          enforce_snapshot(super_root);
        }
      }
    }

    Super_Hash_Trie(const Hash_Trie_Super_Node * const super_node)
      : m_super_root(super_node)
    {
    }

    const Hash_Trie_Super_Node * isnapshot() const {
      const Hash_Trie_Super_Node * super_root = m_super_root.load(std::memory_order_acquire);
      enforce_snapshot(super_root);
      return super_root;
    }

    void enforce_snapshot(const Hash_Trie_Super_Node * &super_root) const {
      while (super_root && !super_root->increment_refs())
        super_root = m_super_root.load(std::memory_order_acquire);
    }

    static bool CAS(std::atomic<const Hash_Trie_Super_Node *> &atomic_value, const Hash_Trie_Super_Node * &expected, const Hash_Trie_Super_Node * desired, const std::memory_order success = std::memory_order_seq_cst, const std::memory_order failure = std::memory_order_seq_cst) {
      if (atomic_value.compare_exchange_strong(expected, desired, success, failure)) {
        if (expected)
          expected->decrement_refs();
        return true;
      }
      else {
        if (desired)
          desired->decrement_refs();
        return false;
      }
    }

    std::atomic<const Hash_Trie_Super_Node *> m_super_root;
  };

}

#endif
