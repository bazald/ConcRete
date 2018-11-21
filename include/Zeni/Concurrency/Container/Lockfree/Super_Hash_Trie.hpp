#ifndef ZENI_CONCURRENCY_SUPER_HASH_TRIE_HPP
#define ZENI_CONCURRENCY_SUPER_HASH_TRIE_HPP

#include "Intrusive_Shared_Ptr.hpp"

#include <functional>
#include <tuple>

namespace Zeni::Concurrency {

  namespace Super_Hash_Trie_Internal {
    template<int... Is>
    struct seq { };

    template<int N, int... Is>
    struct gen_seq : gen_seq<N - 1, N - 1, Is...> { };

    template<int... Is>
    struct gen_seq<0, Is...> : seq<Is...> { };

    template<typename T, typename F, int... Is>
    void for_each(T&& t, F f, seq<Is...>)
    {
      [[maybe_unused]] auto l = { (f(std::get<Is>(t)), 0)... };
    }

    template<typename... Ts, typename F>
    void for_each_in_tuple(std::tuple<Ts...> const& t, F f)
    {
      for_each(t, f, gen_seq<sizeof...(Ts)>());
    }

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

  template <typename... TYPES>
  class Super_Hash_Trie {
  public:
    typedef std::tuple<TYPES...> Types;

  private:
    class Hash_Trie_Super_Node : public Enable_Intrusive_Sharing {
      Hash_Trie_Super_Node & operator=(const Hash_Trie_Super_Node &rhs) = delete;

    public:
      Hash_Trie_Super_Node() = default;

      Hash_Trie_Super_Node(const Hash_Trie_Super_Node &rhs)
        : m_hash_tries(rhs.m_hash_tries)
      {
      }

      Hash_Trie_Super_Node(TYPES&&... types) {
        initialize(std::forward<TYPES...>(types...));
      }

      bool empty() const {
        bool rv = true;
        Super_Hash_Trie_Internal::for_each_in_tuple(m_hash_tries, [&rv](const auto &t) {rv &= t.empty(); });
        return rv;
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

      template <size_t index>
      bool size_zero() const {
        return std::get<index>(m_hash_tries).size_zero();
      }

      template <size_t index, typename Comparable, typename CHash = typename std::tuple_element<index, Types>::type::Hash, typename CPred = typename std::tuple_element<index, Types>::type::Pred>
      auto looked_up(const Comparable &key) const {
        return std::get<index>(m_hash_tries).template looked_up<Comparable, CHash, CPred>(key);
      }

      template <size_t index1, size_t index2, typename Comparable, typename CHash = typename std::tuple_element<index2, typename std::tuple_element<index1, Types>::type>::type::Hash, typename CPred = typename std::tuple_element<index2, typename std::tuple_element<index1, Types>::type>::type::Pred>
      auto looked_up_2(const Comparable &key) const {
        return std::get<index1>(m_hash_tries).template looked_up<index2, Comparable, CHash, CPred>(key);
      }

      template <size_t index1, size_t index2, typename Comparable1, typename Comparable2, typename CHash1 = typename std::tuple_element<index1, Types>::type::Hash, typename CPred1 = typename std::tuple_element<index1, Types>::type::Pred, typename CHash2 = typename std::tuple_element<index2, typename std::tuple_element<index1, Types>::type::Subtrie::Types>::type::Hash, typename CPred2 = typename std::tuple_element<index2, typename std::tuple_element<index1, Types>::type::Subtrie::Types>::type::Pred>
      auto looked_up_2(const Comparable1 &key, const Comparable2 &value) const {
        return std::get<index1>(m_hash_tries).template looked_up<index2, Comparable1, Comparable2, CHash1, CPred1, CHash2, CPred2>(key, value);
      }

      template <size_t index, typename Key>
      auto inserted(const Key &key) const {
        auto tuple_value = std::get<index>(m_hash_tries).inserted(key);
        Hash_Trie_Super_Node * const updated_node = new Hash_Trie_Super_Node(*this);
        std::get<index>(updated_node->m_hash_tries) = std::get<1>(tuple_value);
        return Super_Hash_Trie_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, updated_node);
      }

      template <size_t index1, size_t index2, typename Key>
      auto inserted_2(const Key &key) const {
        auto tuple_value = std::get<index1>(m_hash_tries).template inserted<index2>(key);
        Hash_Trie_Super_Node * const updated_node = new Hash_Trie_Super_Node(*this);
        std::get<index1>(updated_node->m_hash_tries) = std::get<1>(tuple_value);
        return Super_Hash_Trie_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, updated_node);
      }

      template <size_t index1, size_t index2, typename Key, typename Value>
      auto inserted_2(const Key &key, const Value &value) const {
        auto tuple_value = std::get<index1>(m_hash_tries).template inserted<index2>(key, value);
        Hash_Trie_Super_Node * const updated_node = new Hash_Trie_Super_Node(*this);
        std::get<index1>(updated_node->m_hash_tries) = std::get<1>(tuple_value);
        return Super_Hash_Trie_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, updated_node);
      }

      template <size_t index, typename Key>
      auto erased(const Key &key) const {
        auto tuple_value = std::get<index>(m_hash_tries).erased(key);
        Hash_Trie_Super_Node * const updated_node = new Hash_Trie_Super_Node(*this);
        std::get<index>(updated_node->m_hash_tries) = std::get<1>(tuple_value);
        return Super_Hash_Trie_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, updated_node);
      }

      template <size_t index1, size_t index2, typename Key>
      auto erased_2(const Key &key) const {
        auto tuple_value = std::get<index1>(m_hash_tries).template erased<index2>(key);
        Hash_Trie_Super_Node * const updated_node = new Hash_Trie_Super_Node(*this);
        std::get<index1>(updated_node->m_hash_tries) = std::get<1>(tuple_value);
        return Super_Hash_Trie_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, updated_node);
      }

      template <size_t index1, size_t index2, typename Key, typename Value>
      auto erased_2(const Key &key, const Value &value) const {
        auto tuple_value = std::get<index1>(m_hash_tries).template erased<index2>(key, value);
        Hash_Trie_Super_Node * const updated_node = new Hash_Trie_Super_Node(*this);
        std::get<index1>(updated_node->m_hash_tries) = std::get<1>(tuple_value);
        return Super_Hash_Trie_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, updated_node);
      }

      template <size_t index1, size_t index2, typename Key>
      auto moved(const Key &key) const {
        typedef std::remove_cv_t<std::remove_reference_t<decltype(std::get<index1>(m_hash_tries))>> Hash_Trie_Type;

        const auto snode_index = new typename Hash_Trie_Type::SNode(key);
        Hash_Trie_Super_Node * const updated_node = new Hash_Trie_Super_Node(*this);

        const auto[result1, snapshot1, snode1] = std::get<index1>(m_hash_tries).erased(snode_index);
        if (result1 == Hash_Trie_Type::LNode::Result::Failed_Removal)
          return std::make_tuple(result1, updated_node, Key());

        const auto[result2, snapshot2, snode2] = std::get<index2>(m_hash_tries).inserted(snode_index);
        if (result1 == Hash_Trie_Type::LNode::Result::Failed_Insertion)
          return std::make_tuple(result2, updated_node, Key());

        std::get<index1>(updated_node->m_hash_tries) = snapshot1;
        std::get<index2>(updated_node->m_hash_tries) = snapshot2;

        return std::make_tuple(Hash_Trie_Type::LNode::Result::Successful_Move, updated_node, snode2->key);
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

      std::tuple<TYPES ...> m_hash_tries;
    };

  public:
    typedef Super_Hash_Trie<TYPES...> Snapshot;

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

    bool empty() const {
      const auto super_root = m_super_root.load(std::memory_order_acquire);
      return super_root->empty();
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

    template <size_t index>
    bool size_zero() const {
      const auto super_root = m_super_root.load(std::memory_order_acquire);
      return super_root->template size_zero<index>();
    }

    template <size_t index, typename Comparable, typename CHash = typename std::tuple_element<index, Types>::type::Hash, typename CPred = typename std::tuple_element<index, Types>::type::Pred>
    auto lookup(const Comparable &key) const {
      const Hash_Trie_Super_Node * const super_root = isnapshot();
      const auto found = super_root->template looked_up<index, Comparable, CHash, CPred>(key);
      return std::make_pair(found, Snapshot(super_root));
    }

    template <size_t index1, size_t index2, typename Comparable, typename CHash = typename std::tuple_element<index2, typename std::tuple_element<index1, Types>::type>::type::Hash, typename CPred = typename std::tuple_element<index2, typename std::tuple_element<index1, Types>::type>::type::Pred>
    auto lookup_2(const Comparable &key) const {
      const Hash_Trie_Super_Node * const super_root = isnapshot();
      const auto found = super_root->template looked_up_2<index1, index2, Comparable, CHash, CPred>(key);
      return std::make_pair(found, Snapshot(super_root));
    }

    template <size_t index1, size_t index2, typename Comparable1, typename Comparable2, typename CHash1 = typename std::tuple_element<index1, Types>::type::Hash, typename CPred1 = typename std::tuple_element<index1, Types>::type::Pred, typename CHash2 = typename std::tuple_element<index2, typename std::tuple_element<index1, Types>::type::Subtrie::Types>::type::Hash, typename CPred2 = typename std::tuple_element<index2, typename std::tuple_element<index1, Types>::type::Subtrie::Types>::type::Pred>
    auto lookup_2(const Comparable1 &key, const Comparable2 &value) const {
      const Hash_Trie_Super_Node * const super_root = isnapshot();
      const auto found = super_root->template looked_up_2<index1, index2, Comparable1, Comparable2, CHash1, CPred1, CHash2, CPred2>(key, value);
      return std::make_pair(found, Snapshot(super_root));
    }

    template <size_t index, typename Comparable, typename CHash = typename std::tuple_element<index, Types>::type::Hash, typename CPred = typename std::tuple_element<index, Types>::type::Pred>
    auto looked_up(const Comparable &key) const {
      const Hash_Trie_Super_Node * const super_root = m_super_root.load(std::memory_order_acquire);
      return super_root->template looked_up<index, Comparable, CHash, CPred>(key);
    }

    template <size_t index1, size_t index2, typename Comparable, typename CHash = typename std::tuple_element<index2, typename std::tuple_element<index1, Types>::type>::type::Hash, typename CPred = typename std::tuple_element<index2, typename std::tuple_element<index1, Types>::type>::type::Pred>
    auto looked_up_2(const Comparable &key) const {
      const Hash_Trie_Super_Node * const super_root = m_super_root.load(std::memory_order_acquire);
      return super_root->template looked_up_2<index1, index2, Comparable, CHash, CPred>(key);
    }

    template <size_t index1, size_t index2, typename Comparable1, typename Comparable2, typename CHash1 = typename std::tuple_element<index1, Types>::type::Hash, typename CPred1 = typename std::tuple_element<index1, Types>::type::Pred, typename CHash2 = typename std::tuple_element<index2, typename std::tuple_element<index1, Types>::type::Subtrie::Types>::type::Hash, typename CPred2 = typename std::tuple_element<index2, typename std::tuple_element<index1, Types>::type::Subtrie::Types>::type::Pred>
    auto looked_up_2(const Comparable1 &key, const Comparable2 &value) const {
      const Hash_Trie_Super_Node * const super_root = m_super_root.load(std::memory_order_acquire);
      return super_root->template looked_up_2<index1, index2, Comparable1, Comparable2, CHash1, CPred2, CHash2, CPred2>(key, value);
    }

    template <size_t index, typename Key>
    auto insert(const Key &key) {
      return iinsert<index>(key);
    }

    template <size_t index1, size_t index2, typename Key>
    auto insert_2(const Key &key) {
      return iinsert_2<index1, index2>(key);
    }

    template <size_t index1, size_t index2, typename Key, typename Value>
    auto insert_2(const Key &key, const Value &value) {
      return iinsert_2<index1, index2>(key, value);
    }

    template <size_t index, typename Key>
    auto inserted(const Key &key) const {
      return iinserted<index>(key);
    }

    template <size_t index1, size_t index2, typename Key>
    auto inserted_2(const Key &key) const {
      return iinserted_2<index1, index2>(key);
    }

    template <size_t index1, size_t index2, typename Key, typename Value>
    auto inserted_2(const Key &key, const Value &value) const {
      return iinserted_2<index1, index2>(key, value);
    }

    template <size_t index, typename Key>
    auto erase(const Key &key) {
      const auto tuple_value = ierase<index>(key);
      return std::make_tuple(std::get<0>(tuple_value), std::get<1>(tuple_value), std::get<2>(tuple_value));
    }

    template <size_t index1, size_t index2, typename Key>
    auto erase_2(const Key &key) {
      const auto tuple_value = ierase_2<index1, index2>(key);
      return std::make_tuple(std::get<0>(tuple_value), std::get<1>(tuple_value), std::get<2>(tuple_value));
    }

    template <size_t index1, size_t index2, typename Key, typename Value>
    auto erase_2(const Key &key, const Value &value) {
      const auto tuple_value = ierase_2<index1, index2>(key, value);
      return std::make_tuple(std::get<0>(tuple_value), std::get<1>(tuple_value), std::get<2>(tuple_value));
    }

    template <size_t index, typename Key>
    auto erased(const Key &key) const {
      const auto tuple_value = ierased<index>(key);
      return std::make_tuple(std::get<0>(tuple_value), std::get<1>(tuple_value), std::get<2>(tuple_value));
    }

    template <size_t index1, size_t index2, typename Key>
    auto erased_2(const Key &key) const {
      const auto tuple_value = ierased_2<index1, index2>(key);
      return std::make_tuple(std::get<0>(tuple_value), std::get<1>(tuple_value), std::get<2>(tuple_value));
    }

    template <size_t index1, size_t index2, typename Key, typename Value>
    auto erased_2(const Key &key, const Value &value) const {
      const auto tuple_value = ierased_2<index1, index2>(key, value);
      return std::make_tuple(std::get<0>(tuple_value), std::get<1>(tuple_value), std::get<2>(tuple_value));
    }

    template <size_t index1, size_t index2, typename Key>
    auto move(const Key &key) {
      return imove<index1, index2>(key);
    }

    template <size_t index1, size_t index2, typename Key>
    auto moved(const Key &key) const {
      return imoved<index1, index2>(key);
    }

    Snapshot snapshot() const {
      return isnapshot();
    }

    template <size_t index>
    auto snapshot() const {
      const Hash_Trie_Super_Node * super_root = m_super_root.load(std::memory_order_acquire);
      return super_root->template snapshot<index>();
    }

    template <size_t index1, size_t index2>
    auto snapshot_2() const {
      const Hash_Trie_Super_Node * super_root = m_super_root.load(std::memory_order_acquire);
      return super_root->template snapshot<index1>().template snapshot<index2>();
    }

    template <size_t index1, size_t index2>
    auto lookup_snapshot(const typename std::tuple_element<index1, Types>::type::Key &key) const {
      const Hash_Trie_Super_Node * super_root = m_super_root.load(std::memory_order_acquire);
      return super_root->template snapshot<index1>().template lookup_snapshot<index2>(key);
    }

  private:
    template <size_t index, typename Key>
    auto iinsert(const Key &key) {
      const Hash_Trie_Super_Node * super_root = isnapshot();
      for (;;) {
        const auto tuple_value = super_root->template inserted<index>(key);
        if (std::get<1>(tuple_value))
          std::get<1>(tuple_value)->increment_refs();
        if (super_root)
          super_root->decrement_refs();
        if (CAS(m_super_root, super_root, std::get<1>(tuple_value), std::memory_order_release, std::memory_order_acquire))
          return Super_Hash_Trie_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, Snapshot(std::get<1>(tuple_value)));
        else {
          if (std::get<1>(tuple_value))
            std::get<1>(tuple_value)->decrement_refs();
          enforce_snapshot(super_root);
        }
      }
    }

    template <size_t index1, size_t index2, typename Key>
    auto iinsert_2(const Key &key) {
      const Hash_Trie_Super_Node * super_root = isnapshot();
      for (;;) {
        const auto tuple_value = super_root->template inserted_2<index1, index2>(key);
        if (std::get<1>(tuple_value))
          std::get<1>(tuple_value)->increment_refs();
        if (super_root)
          super_root->decrement_refs();
        if (CAS(m_super_root, super_root, std::get<1>(tuple_value), std::memory_order_release, std::memory_order_acquire))
          return Super_Hash_Trie_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, Snapshot(std::get<1>(tuple_value)));
        else {
          if (std::get<1>(tuple_value))
            std::get<1>(tuple_value)->decrement_refs();
          enforce_snapshot(super_root);
        }
      }
    }

    template <size_t index1, size_t index2, typename Key, typename Value>
    auto iinsert_2(const Key &key, const Value &value) {
      const Hash_Trie_Super_Node * super_root = isnapshot();
      for (;;) {
        const auto tuple_value = super_root->template inserted_2<index1, index2>(key, value);
        if (std::get<1>(tuple_value))
          std::get<1>(tuple_value)->increment_refs();
        if (super_root)
          super_root->decrement_refs();
        if (CAS(m_super_root, super_root, std::get<1>(tuple_value), std::memory_order_release, std::memory_order_acquire))
          return Super_Hash_Trie_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, Snapshot(std::get<1>(tuple_value)));
        else {
          if (std::get<1>(tuple_value))
            std::get<1>(tuple_value)->decrement_refs();
          enforce_snapshot(super_root);
        }
      }
    }

    template <size_t index, typename Key>
    auto iinserted(const Key &key) const {
      const Hash_Trie_Super_Node * const super_root = m_super_root.load(std::memory_order_acquire);
      const auto tuple_value = super_root->template inserted<index>(key);
      return Super_Hash_Trie_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, Snapshot(std::get<1>(tuple_value)));
    }

    template <size_t index1, size_t index2, typename Key>
    auto iinserted_2(const Key &key) const {
      const Hash_Trie_Super_Node * const super_root = m_super_root.load(std::memory_order_acquire);
      const auto tuple_value = super_root->template inserted_2<index1, index2>(key);
      return Super_Hash_Trie_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, Snapshot(std::get<1>(tuple_value)));
    }

    template <size_t index1, size_t index2, typename Key, typename Value>
    auto iinserted_2(const Key &key, const Value &value) const {
      const Hash_Trie_Super_Node * const super_root = m_super_root.load(std::memory_order_acquire);
      const auto tuple_value = super_root->template inserted_2<index1, index2>(key, value);
      return Super_Hash_Trie_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, Snapshot(std::get<1>(tuple_value)));
    }

    template <size_t index, typename Key>
    auto ierase(const Key &key) {
      const Hash_Trie_Super_Node * super_root = isnapshot();
      for (;;) {
        const auto tuple_value = super_root->template erased<index>(key);
        if (std::get<1>(tuple_value))
          std::get<1>(tuple_value)->increment_refs();
        if (super_root)
          super_root->decrement_refs();
        if (CAS(m_super_root, super_root, std::get<1>(tuple_value), std::memory_order_release, std::memory_order_acquire))
          return Super_Hash_Trie_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, Snapshot(std::get<1>(tuple_value)));
        else {
          if (std::get<1>(tuple_value))
            std::get<1>(tuple_value)->decrement_refs();
          enforce_snapshot(super_root);
        }
      }
    }

    template <size_t index1, size_t index2, typename Key>
    auto ierase_2(const Key &key) {
      const Hash_Trie_Super_Node * super_root = isnapshot();
      for (;;) {
        const auto tuple_value = super_root->template erased_2<index1, index2>(key);
        if (std::get<1>(tuple_value))
          std::get<1>(tuple_value)->increment_refs();
        if (super_root)
          super_root->decrement_refs();
        if (CAS(m_super_root, super_root, std::get<1>(tuple_value), std::memory_order_release, std::memory_order_acquire))
          return Super_Hash_Trie_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, Snapshot(std::get<1>(tuple_value)));
        else {
          if (std::get<1>(tuple_value))
            std::get<1>(tuple_value)->decrement_refs();
          enforce_snapshot(super_root);
        }
      }
    }

    template <size_t index1, size_t index2, typename Key, typename Value>
    auto ierase_2(const Key &key, const Value &value) {
      const Hash_Trie_Super_Node * super_root = isnapshot();
      for (;;) {
        const auto tuple_value = super_root->template erased_2<index1, index2>(key, value);
        if (std::get<1>(tuple_value))
          std::get<1>(tuple_value)->increment_refs();
        if (super_root)
          super_root->decrement_refs();
        if (CAS(m_super_root, super_root, std::get<1>(tuple_value), std::memory_order_release, std::memory_order_acquire))
          return Super_Hash_Trie_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, Snapshot(std::get<1>(tuple_value)));
        else {
          if (std::get<1>(tuple_value))
            std::get<1>(tuple_value)->decrement_refs();
          enforce_snapshot(super_root);
        }
      }
    }

    template <size_t index, typename Key>
    auto ierased(const Key &key) const {
      const Hash_Trie_Super_Node * const super_root = m_super_root.load(std::memory_order_acquire);
      const auto tuple_value = super_root->template erased<index>(key);
      return Super_Hash_Trie_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, Snapshot(std::get<1>(tuple_value)));
    }

    template <size_t index1, size_t index2, typename Key>
    auto ierased_2(const Key &key) const {
      const Hash_Trie_Super_Node * const super_root = m_super_root.load(std::memory_order_acquire);
      const auto tuple_value = super_root->template erased_2<index1, index2>(key);
      return Super_Hash_Trie_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, Snapshot(std::get<1>(tuple_value)));
    }

    template <size_t index1, size_t index2, typename Key, typename Value>
    auto ierased_2(const Key &key, const Value &value) const {
      const Hash_Trie_Super_Node * const super_root = m_super_root.load(std::memory_order_acquire);
      const auto tuple_value = super_root->template erased_2<index1, index2>(key, value);
      return Super_Hash_Trie_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, Snapshot(std::get<1>(tuple_value)));
    }

    template <size_t index1, size_t index2, typename Key>
    auto imove(const Key &key) {
      const Hash_Trie_Super_Node * super_root = isnapshot();
      for (;;) {
        const auto tuple_value = super_root->template moved<index1, index2>(key);
        if (std::get<1>(tuple_value))
          std::get<1>(tuple_value)->increment_refs();
        if (super_root)
          super_root->decrement_refs();
        if (CAS(m_super_root, super_root, std::get<1>(tuple_value), std::memory_order_release, std::memory_order_acquire))
          return Super_Hash_Trie_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, Snapshot(std::get<1>(tuple_value)));
        else {
          if (std::get<1>(tuple_value))
            std::get<1>(tuple_value)->decrement_refs();
          enforce_snapshot(super_root);
        }
      }
    }

    template <size_t index1, size_t index2, typename Key>
    auto imoved(const Key &key) const {
      const Hash_Trie_Super_Node * const super_root = m_super_root.load(std::memory_order_acquire);
      const auto tuple_value = super_root->template moved<index1, index2>(key);
      return Super_Hash_Trie_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, Snapshot(std::get<1>(tuple_value)));
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
