#ifndef ZENI_CONCURRENCY_HASH_TRIE_S2_HPP
#define ZENI_CONCURRENCY_HASH_TRIE_S2_HPP

#include "Intrusive_Shared_Ptr.hpp"

#include <array>
#include <cstring>
#include <functional>
#include <stack>

namespace Zeni::Concurrency {

  namespace Hash_Trie_S2_Internal {

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

    template <typename INT_TYPE>
    constexpr static INT_TYPE hamming(const INT_TYPE value = std::numeric_limits<INT_TYPE>::max()) {
      return (value & 0x1) + ((value >> 1) ? hamming(value >> 1) : 0);
    }

    template <typename INT_TYPE>
    constexpr static INT_TYPE unhamming(const INT_TYPE value) {
      INT_TYPE rv = 0;
      for (INT_TYPE i = 0; i != value; ++i)
        rv |= INT_TYPE(0x1) << i;
      return rv;
    }

    template <typename SUMMABLE_TYPE>
    constexpr static SUMMABLE_TYPE sum(const SUMMABLE_TYPE lhs, const SUMMABLE_TYPE rhs) {
      return lhs + rhs;
    }

    template <typename INT_TYPE>
    constexpr static INT_TYPE log2(const INT_TYPE value) {
      return value > 0x1 ? 1 + log2(value >> 1) : 0;
    }

    struct Main_Node : public Enable_Intrusive_Sharing {
    private:
      Main_Node(const Main_Node &) = delete;
      Main_Node & operator=(const Main_Node &) = delete;

    public:
      Main_Node() = default;
    };

    template <typename KEY, typename SUBTRIE>
    struct ZENI_CONCURRENCY_CACHE_ALIGN Singleton_Node : public Main_Node {
    private:
      Singleton_Node(const Singleton_Node &) = delete;
      Singleton_Node & operator=(const Singleton_Node &) = delete;

      template <typename KEY_TYPE, typename SUBTRIE_TYPE>
      Singleton_Node(KEY_TYPE &&key_, SUBTRIE_TYPE &&subtrie_) : key(std::forward<KEY_TYPE>(key_)), subtrie(std::forward<SUBTRIE_TYPE>(subtrie_)) {}

    public:
      template <size_t index, typename KEY_TYPE, typename VALUE_TYPE>
      static auto Create_Insert(KEY_TYPE &&key_, VALUE_TYPE &&value_) {
        auto tuple_value = SUBTRIE().template inserted<index>(std::forward<VALUE_TYPE>(value_));
        if (std::get<1>(tuple_value).empty()) {
          return Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<Singleton_Node<KEY, SUBTRIE> *>(nullptr));
        }
        else {
          auto new_snode = new Singleton_Node<KEY, SUBTRIE>(std::forward<KEY_TYPE>(key_), std::get<1>(tuple_value));
          return Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, new_snode);
        }
      }

      template <size_t index, typename KEY_TYPE, typename VALUE_TYPE>
      static auto Create_Erase(KEY_TYPE &&key_, VALUE_TYPE &&value_) {
        auto tuple_value = SUBTRIE().template erased<index>(std::forward<VALUE_TYPE>(value_));
        if (std::get<1>(tuple_value).empty()) {
          return Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<Singleton_Node<KEY, SUBTRIE> *>(nullptr));
        }
        else {
          auto new_snode = new Singleton_Node<KEY, SUBTRIE>(std::forward<KEY_TYPE>(key_), std::get<1>(tuple_value));
          return Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, new_snode);
        }
      }

      template <size_t index>
      auto inserted(const KEY &key, const typename std::tuple_element<index, typename SUBTRIE::Types>::type::Key &value) const {
        auto tuple_value = subtrie.template inserted<index>(value);
        if (std::get<1>(tuple_value).empty()) {
          return Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<Singleton_Node<KEY, SUBTRIE> *>(nullptr));
        }
        else {
          auto new_snode = new Singleton_Node<KEY, SUBTRIE>(key, std::move(std::get<1>(tuple_value)));
          return Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, new_snode);
        }
      }

      template <size_t index>
      auto erased(const KEY &key, const typename std::tuple_element<index, typename SUBTRIE::Types>::type::Key &value) const {
        auto tuple_value = subtrie.template erased<index>(value);
        if (std::get<1>(tuple_value).empty()) {
          return Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<Singleton_Node<KEY, SUBTRIE> *>(nullptr));
        }
        else {
          auto new_snode = new Singleton_Node<KEY, SUBTRIE>(key, std::move(std::get<1>(tuple_value)));
          return Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, new_snode);
        }
      }

      const KEY key;
      const SUBTRIE subtrie;
    };

    template <typename HASH_VALUE_TYPE, typename FLAG_TYPE>
    struct ICtrie_Node : public Main_Node {
    private:
      ICtrie_Node(const ICtrie_Node &) = delete;
      ICtrie_Node & operator=(const ICtrie_Node &) = delete;

      static const FLAG_TYPE hamming_max = hamming<FLAG_TYPE>();

    protected:
      ICtrie_Node(const FLAG_TYPE bmp)
        : m_bmp(bmp)
      {
      }

    public:
      static const FLAG_TYPE W = log2(hamming_max);

      static const ICtrie_Node * Create(const FLAG_TYPE bmp, const size_t hamming_value, const std::array<const Main_Node *, hamming_max> &branches) {
        static Factory factory;
        return factory.create(bmp, hamming_value, branches);
      }

      static const ICtrie_Node * Create(const Main_Node * const first, const HASH_VALUE_TYPE first_hash, const Main_Node * const second, const HASH_VALUE_TYPE second_hash, const size_t level) {
        assert(first);
        assert(second);
        assert(first_hash != second_hash);
        assert(level < hamming_max);
        const auto first_flag = flag(first_hash, level);
        const auto second_flag = flag(second_hash, level);
        if (first_flag == second_flag) {
          std::array<const Main_Node *, hamming_max> branches;
          branches[0] = Create(first, first_hash, second, second_hash, level + W);
          return Create(first_flag, 1, branches);
        }
        else {
          std::array<const Main_Node *, hamming_max> branches;
          branches[0] = first_flag < second_flag ? first : second;
          branches[1] = first_flag > second_flag ? first : second;
          return Create(first_flag | second_flag, 2, branches);
        }
      }

      FLAG_TYPE get_bmp() const { return m_bmp; }

      virtual size_t get_hamming_value() const = 0;

      std::pair<FLAG_TYPE, size_t> flagpos(const HASH_VALUE_TYPE hash_value, const size_t level) const {
        const FLAG_TYPE desired_bit = flag(hash_value, level);
        const size_t array_index = hamming(m_bmp & (desired_bit - 1));
        return std::make_pair(desired_bit, array_index);
      }

      virtual const Main_Node * at(const size_t i) const = 0;

      virtual const ICtrie_Node * inserted(const size_t pos, const FLAG_TYPE flag, const Main_Node * const new_branch) const = 0;

      virtual const ICtrie_Node * updated(const size_t pos, const FLAG_TYPE flag, const Main_Node * const new_branch) const = 0;

      virtual const ICtrie_Node * erased(const size_t pos, const FLAG_TYPE flag) const = 0;

    private:
      static const HASH_VALUE_TYPE unhamming_filter = HASH_VALUE_TYPE(unhamming(W));
      FLAG_TYPE m_bmp;

      class Factory {
        Factory(const Factory &) = delete;
        Factory & operator=(const Factory &) = delete;

      public:
        inline Factory();

        const ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE> * create(const FLAG_TYPE bmp, const size_t hamming_value, const std::array<const Main_Node *, hamming_max> &branches) {
          return m_generator[hamming_value](bmp, branches);
        }

      private:
        std::array<std::function<const ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE> *(const FLAG_TYPE, const std::array<const Main_Node *, hamming_max> &)>, sum(hamming_max, FLAG_TYPE(1))> m_generator;
      };

      static FLAG_TYPE flag(const HASH_VALUE_TYPE hash_value, const size_t level) {
        const HASH_VALUE_TYPE shifted_hash = hash_value >> level;
        const HASH_VALUE_TYPE desired_bit_index = shifted_hash & unhamming_filter;
        const FLAG_TYPE desired_bit = FLAG_TYPE(1u) << desired_bit_index;
        return desired_bit;
      }
    };

    template <typename HASH_VALUE_TYPE, typename FLAG_TYPE, size_t HAMMING_VALUE>
    struct Ctrie_Node : public ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE> {
    private:
      Ctrie_Node(const Ctrie_Node &) = delete;
      Ctrie_Node & operator=(const Ctrie_Node &) = delete;

    public:
      static const size_t hamming_value = HAMMING_VALUE;

      Ctrie_Node(const FLAG_TYPE bmp, const std::array<const Main_Node *, hamming<FLAG_TYPE>()> &branches)
        : ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE>(bmp),
        m_branches(reinterpret_cast<const std::array<const Main_Node *, hamming_value> &>(branches)) //< Should always be smaller, safe to copy subset
      {
#ifndef NDEBUG
        for (size_t i = 0; i != hamming_value; ++i)
          assert(m_branches[i]);
#endif
      }

      ~Ctrie_Node() {
        for (auto branch : m_branches)
          branch->decrement_refs();
      }

      size_t get_hamming_value() const override {
        return hamming_value;
      };

      const Main_Node * at(const size_t i) const override {
        assert(i >= 0 && i < m_branches.size());
        return m_branches[i];
      }

      const ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE> * inserted(const size_t pos, const FLAG_TYPE flag, const Main_Node * const new_branch) const override {
        assert(new_branch);
#ifndef NDEBUG
        for (size_t i = 0; i != hamming_value; ++i)
          assert(m_branches[i]);
#endif
        assert(!(this->get_bmp() & flag));
        std::array<const Main_Node *, hamming<FLAG_TYPE>()> new_branches;
        if (hamming_value) {
          std::memcpy(new_branches.data(), m_branches.data(), pos * sizeof(const Main_Node *));
          std::memcpy(new_branches.data() + (pos + 1), m_branches.data() + pos, (hamming_value - pos) * sizeof(const Main_Node *));
          for (size_t i = 0; i != hamming_value; ++i)
            m_branches[i]->increment_refs();
        }
        new_branches[pos] = new_branch;
#ifndef NDEBUG
        for (size_t i = 0; i != hamming_value + 1; ++i)
          assert(new_branches[i]);
#endif
        return ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE>::Create(this->get_bmp() | flag, hamming_value + 1, new_branches);
      }

      const ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE> * updated(const size_t pos, const FLAG_TYPE flag, const Main_Node * const new_branch) const override {
        assert(new_branch);
#ifndef NDEBUG
        for (size_t i = 0; i != hamming_value; ++i)
          assert(m_branches[i]);
#endif
        assert(this->get_bmp() & flag);
        std::array<const Main_Node *, hamming<FLAG_TYPE>()> new_branches;
        if (hamming_value) {
          std::memcpy(new_branches.data(), m_branches.data(), pos * sizeof(const Main_Node *));
          std::memcpy(new_branches.data() + (pos + 1), m_branches.data() + (pos + 1), (hamming_value - pos - 1) * sizeof(const Main_Node *));
          for (size_t i = 0; i != pos; ++i)
            new_branches[i]->increment_refs();
          for (size_t i = pos + 1; i != hamming_value; ++i)
            new_branches[i]->increment_refs();
        }
        new_branches[pos] = new_branch;
#ifndef NDEBUG
        for (size_t i = 0; i != hamming_value; ++i)
          assert(new_branches[i]);
#endif
        return ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE>::Create(this->get_bmp(), hamming_value, new_branches);
      }

      const ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE> * erased(const size_t pos, const FLAG_TYPE flag) const override {
#ifndef NDEBUG
        for (size_t i = 0; i != hamming_value; ++i)
          assert(m_branches[i]);
#endif
        assert(this->get_bmp() & flag);
        std::array<const Main_Node *, hamming<FLAG_TYPE>()> new_branches;
        if (hamming_value) {
          std::memcpy(new_branches.data(), m_branches.data(), pos * sizeof(const Main_Node *));
          std::memcpy(new_branches.data() + pos, m_branches.data() + (pos + 1), (hamming_value - pos - 1) * sizeof(const Main_Node *));
          for (size_t i = 0; i != hamming_value - 1; ++i)
            new_branches[i]->increment_refs();
        }
#ifndef NDEBUG
        for (size_t i = 0; i != hamming_value - 1; ++i)
          assert(new_branches[i]);
#endif
        return ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE>::Create(this->get_bmp() & ~flag, hamming_value - 1, new_branches);
      }

    private:
      const std::array<const Main_Node *, hamming_value> m_branches;
    };

    template <typename HASH_VALUE_TYPE, typename FLAG_TYPE, size_t IN = hamming<FLAG_TYPE>()>
    struct Ctrie_Node_Generator {
      static void Create(std::array<std::function<const ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE> *(const FLAG_TYPE, const std::array<const Main_Node *, hamming<FLAG_TYPE>()> &)>, sum(hamming<FLAG_TYPE>(), FLAG_TYPE(1))> &generator) {
        Ctrie_Node_Generator<HASH_VALUE_TYPE, FLAG_TYPE, IN - 1>::Create(generator);

        generator[IN] = [](const FLAG_TYPE bmp, const std::array<const Main_Node *, hamming<FLAG_TYPE>()> &branches)->ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE> * {
          return new Ctrie_Node<HASH_VALUE_TYPE, FLAG_TYPE, IN>(bmp, branches);
        };
      }
    };

    template <typename HASH_VALUE_TYPE, typename FLAG_TYPE>
    struct Ctrie_Node_Generator<HASH_VALUE_TYPE, FLAG_TYPE, 0> {
      static void Create(std::array<std::function<const ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE> *(const FLAG_TYPE, const std::array<const Main_Node *, hamming<FLAG_TYPE>()> &)>, sum(hamming<FLAG_TYPE>(), FLAG_TYPE(1))> &generator) {
        generator[0] = [](const FLAG_TYPE /*bmp*/, const std::array<const Main_Node *, hamming<FLAG_TYPE>()> &/*branches*/)->ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE> * {
          return nullptr; // new Ctrie_Node<HASH_VALUE_TYPE, 0>(bmp, branches);
        };
      }
    };

    template <typename HASH_VALUE_TYPE, typename FLAG_TYPE>
    ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE>::Factory::Factory() {
      Ctrie_Node_Generator<HASH_VALUE_TYPE, FLAG_TYPE>::Create(m_generator);
    }

    template <typename KEY, typename SUBTRIE, typename PRED>
    struct List_Node : public Main_Node {
    private:
      List_Node(const List_Node &) = delete;
      List_Node & operator=(const List_Node &) = delete;

    public:
      List_Node(const Singleton_Node<KEY, SUBTRIE> * const snode_, List_Node * const next_ = nullptr)
        : snode(snode_), next(next_)
      {
        assert(snode);
      }

      ~List_Node() {
        snode->decrement_refs();
        while (next) {
          const int64_t refs = next->decrement_refs();
          if (refs > 1)
            break;
          List_Node * next_next = next->next;
          next->next = nullptr;
          next = next_next;
        }
      }

      template <size_t index>
      auto inserted(const KEY &key, const typename std::tuple_element<index, typename SUBTRIE::Types>::type::Key &value) const {
        const Singleton_Node<KEY, SUBTRIE> * found = nullptr;
        List_Node * new_head = nullptr;
        List_Node * new_tail = nullptr;
        List_Node * old_head = const_cast<List_Node *>(this);
        for (; old_head; old_head = old_head->next) {
          if (PRED()(old_head->snode->key, key)) {
            found = old_head->snode;
            if (old_head->next) {
              old_head->next->increment_refs();
              if (new_head)
                new_tail->next = old_head->next;
              else {
                new_head = old_head->next;
                new_tail = new_head;
              }
              break;
            }
          }
          else {
            old_head->snode->increment_refs();
            if (new_head)
              new_head = new List_Node(old_head->snode, new_head);
            else {
              new_head = new List_Node(old_head->snode, nullptr);
              new_tail = new_head;
            }
          }
        }
        if (found) {
          auto tuple_value = found->template inserted<index>(key, value);
          auto new_lnode = std::get<1>(tuple_value) ? new List_Node(std::get<1>(tuple_value), new_head) : new_head;
          return Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, new_lnode);
        }
        else {
          auto tuple_value = Singleton_Node<KEY, SUBTRIE>::template Create_Insert<index>(key, value);
          return Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, new List_Node(std::get<1>(tuple_value), new_head));
        }
      }

      template <size_t index>
      auto erased(const KEY &key, const typename std::tuple_element<index, typename SUBTRIE::Types>::type::Key &value) const {
        const Singleton_Node<KEY, SUBTRIE> * found = nullptr;
        List_Node * new_head = nullptr;
        List_Node * new_tail = nullptr;
        List_Node * old_head = const_cast<List_Node *>(this);
        for (; old_head; old_head = old_head->next) {
          if (PRED()(old_head->snode->key, key)) {
            found = old_head->snode;
            if (old_head->next) {
              old_head->next->increment_refs();
              if (new_head)
                new_tail->next = old_head->next;
              else {
                new_head = old_head->next;
                new_tail = new_head;
              }
              break;
            }
          }
          else {
            old_head->snode->increment_refs();
            if (new_head)
              new_head = new List_Node(old_head->snode, new_head);
            else {
              new_head = new List_Node(old_head->snode, nullptr);
              new_tail = new_head;
            }
          }
        }
        if (found) {
          auto tuple_value = found->template erased<index>(key, value);
          auto new_lnode = std::get<1>(tuple_value) ? new List_Node(std::get<1>(tuple_value), new_head) : new_head;
          return Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, new_lnode);
        }
        else {
          auto tuple_value = Singleton_Node<KEY, SUBTRIE>::template Create_Erase<index>(key, value);
          return Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, new List_Node(std::get<1>(tuple_value), new_head));
        }
      }

    public:
      const Singleton_Node<KEY, SUBTRIE> * const snode;
      List_Node * next = nullptr;
    };
  }

  template <typename KEY, typename SUBTRIE, typename HASH = std::hash<KEY>, typename PRED = std::equal_to<KEY>, typename FLAG_TYPE = uint32_t>
  class Hash_Trie_S2 {
  public:
    typedef KEY Key;
    typedef SUBTRIE Subtrie;
    typedef HASH Hash;
    typedef PRED Pred;

    typedef decltype(Hash()(Key())) Hash_Value;
    typedef FLAG_TYPE Flag_Type;

    typedef Hash_Trie_S2_Internal::Main_Node MNode;
    typedef Hash_Trie_S2_Internal::Singleton_Node<Key, Subtrie> SNode;
    typedef Hash_Trie_S2_Internal::ICtrie_Node<Hash_Value, Flag_Type> CNode;
    typedef Hash_Trie_S2_Internal::List_Node<Key, Subtrie, Pred> LNode;

    typedef Hash_Trie_S2<Key, Subtrie, Hash, Pred, Flag_Type> Snapshot;

    class const_iterator {
      struct Level {
        const MNode * mnode = nullptr;
        size_t pos = -1;

        bool operator==(const Level &rhs) const {
          return mnode == rhs.mnode && pos == rhs.pos;
        }
        bool operator!=(const Level &rhs) const {
          return mnode != rhs.mnode || pos != rhs.pos;
        }
      };

    public:
      typedef std::forward_iterator_tag iterator_category;
      typedef std::pair<Key, Subtrie> value_type;
      typedef std::pair<const Key &, const Subtrie &> reference;

      const_iterator() = default;

      const_iterator(const MNode * root)
        : m_root(root)
      {
        for (;;) {
          if (const auto cnode = dynamic_cast<const CNode *>(root)) {
            assert(cnode->get_hamming_value() != 0);
            if (cnode->get_hamming_value() != 1)
              m_level_stack.push({ cnode, size_t(1) });
            root = cnode->at(0);
          }
          else if (const auto lnode = dynamic_cast<const LNode *>(root)) {
            if (lnode->next)
              m_level_stack.push({ lnode->next, size_t(-1) });
            m_level_stack.push({ lnode->snode, size_t(-1) });
            break;
          }
          else if (const auto snode = dynamic_cast<const SNode *>(root)) {
            m_level_stack.push({ snode, size_t(-1) });
            break;
          }
          else
            break;
        }
      }

      reference operator*() const {
        const SNode * const snode = static_cast<const SNode *>(m_level_stack.top().mnode);
        return std::make_pair(std::reference_wrapper<const Key>(snode->key), std::reference_wrapper<const Subtrie>(snode->subtrie));
      }

      const_iterator next() const {
        return ++const_iterator(*this);
      }

      const_iterator & operator++() {
        m_level_stack.pop();
        while (!m_level_stack.empty()) {
          const auto top = m_level_stack.top();
          m_level_stack.pop();
          if (const auto cnode = dynamic_cast<const CNode *>(top.mnode)) {
            if (top.pos + 1 != cnode->get_hamming_value())
              m_level_stack.push({ cnode, top.pos + 1 });
            const auto mnode_next = cnode->at(top.pos);
            if (const auto snode_next = dynamic_cast<const SNode *>(mnode_next)) {
              m_level_stack.push({ snode_next, size_t(0) });
              break;
            }
            else
              m_level_stack.push({ mnode_next, size_t(0) });
          }
          else if (const auto lnode = dynamic_cast<const LNode *>(top.mnode)) {
            if (lnode->next)
              m_level_stack.push({ lnode->next, size_t(-1) });
            m_level_stack.push({ lnode->snode, size_t(-1) });
            break;
          }
          else if (const auto snode = dynamic_cast<const SNode *>(top.mnode)) {
            abort();
          }
          else
            abort();
        }
        return *this;
      }

      const_iterator operator++(int) {
        const_iterator rv(*this);
        ++*this;
        return rv;
      }

      bool operator==(const const_iterator &rhs) const {
        return m_level_stack == rhs.m_level_stack;
      }
      bool operator!=(const const_iterator &rhs) const {
        return m_level_stack != rhs.m_level_stack;
      }

    private:
      Intrusive_Shared_Ptr<const MNode> m_root;
      std::stack<Level> m_level_stack;
    };

    const_iterator cbegin() const {
      return const_iterator(isnapshot());
    }

    const_iterator cend() const {
      return const_iterator();
    }

    const_iterator begin() const {
      return cbegin();
    }

    const_iterator end() const {
      return cend();
    }

    Hash_Trie_S2() = default;

    Hash_Trie_S2(const Hash_Trie_S2 &rhs)
      : m_root(rhs.isnapshot())
    {
    }

    Hash_Trie_S2 & operator=(const Hash_Trie_S2 &rhs) {
      const MNode * root = m_root.load(std::memory_order_acquire);
      const MNode * const new_root = rhs.isnapshot();
      CAS(m_root, root, new_root, std::memory_order_release, std::memory_order_acquire);
      return *this;
    }

    Hash_Trie_S2(Hash_Trie_S2 &&rhs)
      : m_root(rhs.m_root.load(std::memory_order_relaxed))
    {
      rhs.m_root.store(nullptr, std::memory_order_relaxed);
    }

    ~Hash_Trie_S2() {
      const MNode * const mnode = m_root.load(std::memory_order_acquire);
      if (mnode)
        mnode->decrement_refs();
    }

    bool empty() const {
      return !m_root.load(std::memory_order_acquire);
    }

    size_t size() const {
      size_t sz = 0;
      for ([[maybe_unused]] auto &value : *this)
        ++sz;
      return sz;
    }

    bool size_one() const {
      auto it = cbegin();
      const auto iend = cend();
      if (it != iend)
        return ++it == iend;
      return false;
    }

    bool size_zero() const {
      return cbegin() == cend();
    }

    template <size_t index, typename Comparable1, typename Comparable2, typename CHash1 = Hash, typename CPred1 = Pred, typename CHash2 = typename std::tuple_element<index, typename Subtrie::Types>::type::Hash, typename CPred2 = typename std::tuple_element<index, typename Subtrie::Types>::type::Pred>
    auto lookup(const Comparable1 &key, const Comparable2 &value) const {
      const Hash_Value hash_value = CHash1()(key);
      const MNode * const root = isnapshot();
      const auto found = ilookup<index, Comparable1, Comparable2, CPred1, CHash2, CPred2>(root, key, value, hash_value, 0);
      return std::make_pair(found, Snapshot(root));
    }

    template <size_t index, typename Comparable1, typename Comparable2, typename CHash1 = Hash, typename CPred1 = Pred, typename CHash2 = typename std::tuple_element<index, typename Subtrie::Types>::type::Hash, typename CPred2 = typename std::tuple_element<index, typename Subtrie::Types>::type::Pred>
    auto looked_up(const Comparable1 &key, const Comparable2 &value) const {
      const Hash_Value hash_value = CHash1()(key);
      const MNode * const root = m_root.load(std::memory_order_acquire);
      return ilookup<index, Comparable1, Comparable2, CPred1, CHash2, CPred2>(root, key, value, hash_value, 0);
    }

    template <size_t index>
    auto insert(const Key &key, const typename std::tuple_element<index, typename Subtrie::Types>::type::Key &value) {
      return iinsert<index>(key, value);
    }

    template <size_t index>
    auto inserted(const Key &key, const typename std::tuple_element<index, typename Subtrie::Types>::type::Key &value) const {
      return iinserted<index>(key, value);
    }

    template <size_t index>
    auto erase(const Key &key, const typename std::tuple_element<index, typename Subtrie::Types>::type::Key &value) {
      return ierase<index>(key, value);
    }

    template <size_t index>
    auto erased(const Key &key, const typename std::tuple_element<index, typename Subtrie::Types>::type::Key &value) const {
      return ierased<index>(key, value);
    }

    Snapshot snapshot() const {
      return isnapshot();
    }

    template <size_t index, typename Comparable1, typename CHash1 = Hash, typename CPred1 = Pred>
    auto lookup_snapshot(const Comparable1 &key) const {
      const Hash_Value hash_value = CHash1()(key);
      const MNode * const root = m_root.load(std::memory_order_acquire);
      return isnapshot<index, Comparable1, CPred1>(root, key, hash_value, 0);
    }

  private:
    template <size_t index>
    auto iinsert(const Key &key, const typename std::tuple_element<index, typename Subtrie::Types>::type::Key &value) {
      const Hash_Value hash_value = Hash()(key);
      const MNode * root = isnapshot();
      for (;;) {
        auto tuple_value = iinsert<index>(root, key, value, hash_value, 0);
        if (std::get<1>(tuple_value))
          std::get<1>(tuple_value)->increment_refs();
        if (root)
          root->decrement_refs();
        if (CAS(m_root, root, std::get<1>(tuple_value), std::memory_order_release, std::memory_order_acquire))
          return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, Snapshot(std::get<1>(tuple_value)));
        else {
          if (std::get<1>(tuple_value))
            std::get<1>(tuple_value)->decrement_refs();
          enforce_snapshot(root);
        }
      }
    }

    template <size_t index>
    auto ierase(const Key &key, const typename std::tuple_element<index, typename Subtrie::Types>::type::Key &value) {
      const Hash_Value hash_value = Hash()(key);
      const MNode * root = isnapshot();
      for (;;) {
        auto tuple_value = ierase<index>(root, key, value, hash_value, 0);
        if (std::get<1>(tuple_value))
          std::get<1>(tuple_value)->increment_refs();
        if (root)
          root->decrement_refs();
        if (CAS(m_root, root, std::get<1>(tuple_value), std::memory_order_release, std::memory_order_acquire))
          return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, Snapshot(std::get<1>(tuple_value)));
        else {
          if (std::get<1>(tuple_value))
            std::get<1>(tuple_value)->decrement_refs();
          enforce_snapshot(root);
        }
      }
    }

    template <size_t index>
    auto iinserted(const Key &key, const typename std::tuple_element<index, typename Subtrie::Types>::type::Key &value) const {
      const Hash_Value hash_value = Hash()(key);
      const MNode * const root = m_root.load(std::memory_order_acquire);
      auto tuple_value = iinsert<index>(root, key, value, hash_value, 0);
      return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, Snapshot(std::get<1>(tuple_value)));
    }

    template <size_t index>
    auto ierased(const Key &key, const typename std::tuple_element<index, typename Subtrie::Types>::type::Key &value) const {
      const Hash_Value hash_value = Hash()(key);
      const MNode * const root = m_root.load(std::memory_order_acquire);
      auto tuple_value = ierase<index>(root, key, value, hash_value, 0);
      return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, Snapshot(std::get<1>(tuple_value)));
    }

    Hash_Trie_S2(const MNode * const mnode)
      : m_root(mnode)
    {
    }

    const MNode * isnapshot() const {
      const MNode * root = m_root.load(std::memory_order_acquire);
      enforce_snapshot(root);
      return root;
    }

    void enforce_snapshot(const MNode * &root) const {
      while (root && !root->increment_refs())
        root = m_root.load(std::memory_order_acquire);
    }

    template <size_t index, typename Comparable1, typename Comparable2, typename CPred1, typename CHash2, typename CPred2>
    typename std::tuple_element<index, typename Subtrie::Types>::type::Key ilookup(
      const MNode * mnode,
      const Comparable1 &key,
      const Comparable2 &value,
      const Hash_Value &hash_value,
      size_t level) const
    {
      for (;;) {
        if (const auto cnode = dynamic_cast<const CNode *>(mnode)) {
          const auto[flag, pos] = cnode->flagpos(hash_value, level);
          const MNode * const next = cnode->get_bmp() & flag ? cnode->at(pos) : nullptr;
          if (!next)
            return typename std::tuple_element<index, typename Subtrie::Types>::type::Key();
          mnode = next;
          level += CNode::W;
          continue;
        }
        else if (auto lnode = dynamic_cast<const LNode *>(mnode)) {
          do {
            if (CPred1()(lnode->snode->key, key))
              return lnode->snode->subtrie.template looked_up<index, Comparable2, CHash2, CPred2>(value);
            else
              lnode = lnode->next;
          } while (lnode);
          return typename std::tuple_element<index, typename Subtrie::Types>::type::Key();
        }
        else if (auto snode = dynamic_cast<const SNode *>(mnode))
          return CPred1()(snode->key, key) ? snode->subtrie.template looked_up<index, Comparable2, CHash2, CPred2>(value) : typename std::tuple_element<index, typename Subtrie::Types>::type::Key();
        else
          return typename std::tuple_element<index, typename Subtrie::Types>::type::Key();
      }
    }

    template <size_t index, typename Comparable1, typename CPred1>
    typename std::tuple_element<index, typename Subtrie::Types>::type::Snapshot isnapshot(
      const MNode * mnode,
      const Comparable1 &key,
      const Hash_Value &hash_value,
      size_t level) const
    {
      for (;;) {
        if (const auto cnode = dynamic_cast<const CNode *>(mnode)) {
          const auto[flag, pos] = cnode->flagpos(hash_value, level);
          const MNode * const next = cnode->get_bmp() & flag ? cnode->at(pos) : nullptr;
          if (!next)
            return typename std::tuple_element<index, typename Subtrie::Types>::type::Snapshot();
          mnode = next;
          level += CNode::W;
          continue;
        }
        else if (auto lnode = dynamic_cast<const LNode *>(mnode)) {
          do {
            if (CPred1()(lnode->snode->key, key))
              return lnode->snode->subtrie.template snapshot<index>();
            else
              lnode = lnode->next;
          } while (lnode);
          return typename std::tuple_element<index, typename Subtrie::Types>::type::Snapshot();
        }
        else if (auto snode = dynamic_cast<const SNode *>(mnode))
          return CPred1()(snode->key, key) ? snode->subtrie.template snapshot<index>() : typename std::tuple_element<index, typename Subtrie::Types>::type::Snapshot();
        else
          return typename std::tuple_element<index, typename Subtrie::Types>::type::Snapshot();
      }
    }

    template <size_t index>
    auto iinsert(
      const MNode * const mnode,
      const Key &key,
      const typename std::tuple_element<index, typename Subtrie::Types>::type::Key &value,
      const Hash_Value &hash_value,
      const size_t level) const
    {
      if (const auto cnode = dynamic_cast<const CNode *>(mnode)) {
        const auto[flag, pos] = cnode->flagpos(hash_value, level);
        const MNode * const next = cnode->get_bmp() & flag ? cnode->at(pos) : nullptr;
        if (!next) {
          auto tuple_value = SNode::template Create_Insert<index>(key, value);
          return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<const MNode *>(cnode->inserted(pos, flag, std::get<1>(tuple_value))));
        }
        auto tuple_value = iinsert<index>(next, key, value, hash_value, level + CNode::W);
        if (!std::get<1>(tuple_value)) {
          const auto new_cnode = cnode->erased(pos, flag);
          if (!new_cnode)
            return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<const MNode *>(nullptr));
          else if (new_cnode->get_hamming_value() != 1)
            return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<const MNode *>(new_cnode));
          else {
            const auto new_mnode = new_cnode->at(0);
            if (dynamic_cast<const CNode *>(new_mnode))
              return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<const MNode *>(new_cnode));
            else {
              new_mnode->increment_refs();
              delete new_cnode;
              return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, new_mnode);
            }
          }
        }
        else
          return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<const MNode *>(cnode->updated(pos, flag, std::get<1>(tuple_value))));
      }
      else if (auto lnode = dynamic_cast<const LNode *>(mnode)) {
        const Hash_Value lnode_hash = Hash()(lnode->snode->key);
        if (lnode_hash != hash_value) {
          lnode->increment_refs();
          auto tuple_value = SNode::template Create_Insert<index>(key, value);
          return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<const MNode *>(CNode::Create(std::get<1>(tuple_value), hash_value, lnode, lnode_hash, level)));
        }
        else {
          auto tuple_value = lnode->template inserted<index>(key, value);
          if (!std::get<1>(tuple_value)->next) {
            std::get<1>(tuple_value)->snode->increment_refs();
            std::get<1>(tuple_value)->decrement_refs();
            return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<const MNode *>(std::get<1>(tuple_value)->snode));
          }
          else
            return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<const MNode *>(std::get<1>(tuple_value)));
        }
      }
      else if (auto snode = dynamic_cast<const SNode *>(mnode)) {
        const Hash_Value snode_hash = Hash()(snode->key);
        if (snode_hash != hash_value) {
          snode->increment_refs();
          auto tuple_value = SNode::template Create_Insert<index>(key, value);
          return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<const MNode *>(CNode::Create(std::get<1>(tuple_value), hash_value, snode, snode_hash, level)));
        }
        else if (Pred()(snode->key, key)) {
          auto tuple_value = snode->template inserted<index>(key, value);
          return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<const MNode *>(std::get<1>(tuple_value)));
        }
        else {
          snode->increment_refs();
          auto tuple_value = SNode::template Create_Insert<index>(key, value);
          return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<const MNode *>(new LNode(std::get<1>(tuple_value), new LNode(snode))));
        }
      }
      else {
        auto tuple_value = SNode::template Create_Insert<index>(key, value);
        return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<const MNode *>(std::get<1>(tuple_value)));
      }
    }

    template <size_t index>
    auto ierase(
      const MNode * const mnode,
      const Key &key,
      const typename std::tuple_element<index, typename Subtrie::Types>::type::Key &value,
      const Hash_Value &hash_value,
      const size_t level) const
    {
      if (const auto cnode = dynamic_cast<const CNode *>(mnode)) {
        const auto[flag, pos] = cnode->flagpos(hash_value, level);
        const MNode * const next = cnode->get_bmp() & flag ? cnode->at(pos) : nullptr;
        if (!next) {
          auto tuple_value = SNode::template Create_Erase<index>(key, value);
          return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<const MNode *>(cnode->inserted(pos, flag, std::get<1>(tuple_value))));
        }
        auto tuple_value = ierase<index>(next, key, value, hash_value, level + CNode::W);
        if (!std::get<1>(tuple_value)) {
          const auto new_cnode = cnode->erased(pos, flag);
          if (!new_cnode)
            return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<const MNode *>(nullptr));
          else if (new_cnode->get_hamming_value() != 1)
            return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<const MNode *>(new_cnode));
          else {
            const auto new_mnode = new_cnode->at(0);
            if (dynamic_cast<const CNode *>(new_mnode))
              return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<const MNode *>(new_cnode));
            else {
              new_mnode->increment_refs();
              delete new_cnode;
              return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, new_mnode);
            }
          }
        }
        else
          return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<const MNode *>(cnode->updated(pos, flag, std::get<1>(tuple_value))));
      }
      else if (auto lnode = dynamic_cast<const LNode *>(mnode)) {
        const Hash_Value lnode_hash = Hash()(lnode->snode->key);
        if (lnode_hash != hash_value) {
          lnode->increment_refs();
          auto tuple_value = SNode::template Create_Erase<index>(key, value);
          return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<const MNode *>(CNode::Create(std::get<1>(tuple_value), hash_value, lnode, lnode_hash, level)));
        }
        else {
          auto tuple_value = lnode->template erased<index>(key, value);
          if (!std::get<1>(tuple_value)->next) {
            std::get<1>(tuple_value)->snode->increment_refs();
            std::get<1>(tuple_value)->decrement_refs();
            return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<const MNode *>(std::get<1>(tuple_value)->snode));
          }
          else
            return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<const MNode *>(std::get<1>(tuple_value)));
        }
      }
      else if (auto snode = dynamic_cast<const SNode *>(mnode)) {
        const Hash_Value snode_hash = Hash()(snode->key);
        if (snode_hash != hash_value) {
          snode->increment_refs();
          auto tuple_value = SNode::template Create_Erase<index>(key, value);
          return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<const MNode *>(CNode::Create(std::get<1>(tuple_value), hash_value, snode, snode_hash, level)));
        }
        else if (Pred()(snode->key, key)) {
          auto tuple_value = snode->template erased<index>(key, value);
          return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<const MNode *>(std::get<1>(tuple_value)));
        }
        else {
          snode->increment_refs();
          auto tuple_value = SNode::template Create_Erase<index>(key, value);
          return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<const MNode *>(new LNode(std::get<1>(tuple_value), new LNode(snode))));
        }
      }
      else {
        auto tuple_value = SNode::template Create_Erase<index>(key, value);
        return Hash_Trie_S2_Internal::Update_Tuple_1<std::tuple_size<decltype(tuple_value)>::value>::updated(tuple_value, static_cast<const MNode *>(std::get<1>(tuple_value)));
      }
    }

    template <typename VALUE_TYPE, typename DESIRED>
    static bool CAS(std::atomic<VALUE_TYPE *> &atomic_value, VALUE_TYPE * &expected, const DESIRED * desired, const std::memory_order success = std::memory_order_seq_cst, const std::memory_order failure = std::memory_order_seq_cst) {
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

    ZENI_CONCURRENCY_CACHE_ALIGN std::atomic<const MNode *> m_root = nullptr;
  };

}

#endif
