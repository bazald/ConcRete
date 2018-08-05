#ifndef ZENI_CONCURRENCY_CTRIE_HPP
#define ZENI_CONCURRENCY_CTRIE_HPP

#include "Intrusive_Shared_Ptr.hpp"

#include <array>
#include <cstring>
#include <functional>

namespace Zeni::Concurrency {

  namespace Ctrie_Internal {

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

    class Generation : public Enable_Intrusive_Sharing<Generation> {};

    struct Main_Node : public Enable_Intrusive_Sharing<Main_Node> {
    private:
      Main_Node(const Main_Node &) = delete;
      Main_Node & operator=(const Main_Node &) = delete;

    public:
      Main_Node() = default;

      ~Main_Node() {
        if (prev)
          prev->decrement_refs();
      }

      const Main_Node * prev = nullptr;
    };

    struct Branch : public Enable_Intrusive_Sharing<Branch> {
    private:
      Branch(const Branch &) = delete;
      Branch & operator=(const Branch &) = delete;

    public:
      Branch() = default;

      virtual Branch * resurrect() = 0;
    };

    struct Tomb_Node : public Main_Node {
    private:
      Tomb_Node(const Tomb_Node &) = delete;
      Tomb_Node  &operator=(const Tomb_Node &) = delete;

    public:
      Tomb_Node() = default;

      Tomb_Node(Branch * const branch_) : branch(branch_) {}

      ~Tomb_Node() {
        branch->decrement_refs();
      }

      Branch * const branch;
    };

    struct Indirection_Node : public Branch {
    private:
      Indirection_Node(const Indirection_Node &) = delete;
      Indirection_Node & operator=(const Indirection_Node &) = delete;

    public:
      Indirection_Node(const Main_Node * main_, const Generation * const gen_)
        : main(main_),
        gen(gen_)
      {
      }

      ~Indirection_Node() {
        main.load(std::memory_order_acquire)->decrement_refs();
        gen.load(std::memory_order_relaxed)->decrement_refs();
      }

      Branch * resurrect() override {
        if (const auto tomb_node = dynamic_cast<const Tomb_Node *>(main.load(std::memory_order_acquire))) {
          tomb_node->branch->increment_refs();
          return tomb_node->branch;
        }
        else {
          increment_refs();
          return this;
        }
      }

      ZENI_CONCURRENCY_CACHE_ALIGN std::atomic<const Main_Node *> main;
      ZENI_CONCURRENCY_CACHE_ALIGN std::atomic<const Generation *> gen;
    };

    template <typename HASH_VALUE_TYPE>
    struct ICtrie_Node : public Main_Node {
    private:
      ICtrie_Node(const ICtrie_Node &) = delete;
      ICtrie_Node & operator=(const ICtrie_Node &) = delete;

      static const HASH_VALUE_TYPE hamming_max = hamming<HASH_VALUE_TYPE>();

    protected:
      ICtrie_Node(const HASH_VALUE_TYPE bmp)
        : m_bmp(bmp)
      {
      }

    public:
      static const HASH_VALUE_TYPE W = log2(hamming_max);

      static const ICtrie_Node * Create(const HASH_VALUE_TYPE bmp, const size_t hamming_value, const std::array<Branch *, hamming_max> &branches) {
        static Factory factory;
        return factory.create(bmp, hamming_value, branches);
      }

      static const ICtrie_Node * Create(Branch * const first, const HASH_VALUE_TYPE first_hash, Branch * const second, const HASH_VALUE_TYPE second_hash, const size_t level) {
        assert(first_hash != second_hash);
        assert(level < hamming_max);
        const auto first_flag = flag(first_hash, level);
        const auto second_flag = flag(second_hash, level);
        if (first_flag == second_flag) {
          std::array<Branch *, hamming_max> branches;
          branches[0] = new Indirection_Node(Create(first, first_hash, second, second_hash, level + W), new Generation);
          return Create(first_flag, 1, branches);
        }
        else {
          std::array<Branch *, hamming_max> branches;
          branches[0] = first_flag < second_flag ? first : second;
          branches[1] = first_flag > second_flag ? first : second;
          return Create(first_flag | second_flag, 2, branches);
        }
      }

      HASH_VALUE_TYPE get_bmp() const { return m_bmp; }

      virtual size_t get_hamming_value() const = 0;

      std::pair<HASH_VALUE_TYPE, size_t> flagpos(const HASH_VALUE_TYPE hash_value, const size_t level) const {
        const HASH_VALUE_TYPE desired_bit = flag(hash_value, level);
        const size_t array_index = hamming(m_bmp & (desired_bit - 1));
        return std::make_pair(desired_bit, array_index);
      }

      virtual Branch * at(const size_t i) const = 0;

      virtual const ICtrie_Node * inserted(const size_t pos, const HASH_VALUE_TYPE flag, Branch * const new_branch) const = 0;

      virtual const ICtrie_Node * updated(const size_t pos, const HASH_VALUE_TYPE flag, Branch * const new_branch) const = 0;

      virtual const ICtrie_Node * removed(const size_t pos, const HASH_VALUE_TYPE flag) const = 0;

      const Main_Node * to_compressed(const size_t level) const {
        std::array<Branch *, hamming_max> branches;
        for (size_t i = 0; i != get_hamming_value(); ++i)
          branches[i] = at(i)->resurrect();
        return Create(m_bmp, get_hamming_value(), branches)->to_contracted(level);
      }

      const Main_Node * to_contracted(const size_t level) const {
        if (level && get_hamming_value() == 1) {
          Branch * const branch = at(0);
          branch->increment_refs();
          delete this;
          return new Tomb_Node(branch);
        }
        else
          return this;
      }

    private:
      static const HASH_VALUE_TYPE unhamming_filter = unhamming(W);
      HASH_VALUE_TYPE m_bmp;

      class Factory {
        Factory(const Factory &) = delete;
        Factory & operator=(const Factory &) = delete;

      public:
        inline Factory();

        const ICtrie_Node<HASH_VALUE_TYPE> * create(const HASH_VALUE_TYPE bmp, const size_t hamming_value, const std::array<Branch *, hamming_max> &branches) {
          return m_generator[hamming_value](bmp, branches);
        }

      private:
        std::array<std::function<const ICtrie_Node<HASH_VALUE_TYPE> *(const HASH_VALUE_TYPE, const std::array<Branch *, hamming_max> &)>, sum(hamming_max, HASH_VALUE_TYPE(1))> m_generator;
      };

      static HASH_VALUE_TYPE flag(const HASH_VALUE_TYPE hash_value, const size_t level) {
        const HASH_VALUE_TYPE shifted_hash = hash_value >> level;
        const HASH_VALUE_TYPE desired_bit_index = shifted_hash & unhamming_filter;
        const HASH_VALUE_TYPE desired_bit = HASH_VALUE_TYPE(1u) << desired_bit_index;
        return desired_bit;
      }
    };

    template <typename HASH_VALUE_TYPE, size_t HAMMING_VALUE>
    struct Ctrie_Node : public ICtrie_Node<HASH_VALUE_TYPE> {
    private:
      Ctrie_Node(const Ctrie_Node &) = delete;
      Ctrie_Node & operator=(const Ctrie_Node &) = delete;

    public:
      static const size_t hamming_value = HAMMING_VALUE;

      Ctrie_Node(const HASH_VALUE_TYPE bmp, const std::array<Branch *, hamming<HASH_VALUE_TYPE>()> &branches)
        : ICtrie_Node<HASH_VALUE_TYPE>(bmp),
        m_branches(reinterpret_cast<const std::array<Branch *, hamming_value> &>(branches)) //< Should always be smaller, safe to copy subset
      {
      }

      ~Ctrie_Node() {
        for (auto branch : m_branches)
          branch->decrement_refs();
      }

      size_t get_hamming_value() const override {
        return hamming_value;
      };

      Branch * at(const size_t i) const override {
        assert(i >= 0 && i < m_branches.size());
        return m_branches[i];
      }

      const ICtrie_Node<HASH_VALUE_TYPE> * inserted(const size_t pos, const HASH_VALUE_TYPE flag, Branch * const new_branch) const override {
        assert(!(get_bmp() & flag));
        std::array<Branch *, hamming<HASH_VALUE_TYPE>()> new_branches;
        if (hamming_value) {
          std::memcpy(new_branches.data(), m_branches.data(), pos * sizeof(Branch *));
          std::memcpy(new_branches.data() + (pos + 1), m_branches.data() + pos, (hamming_value - pos) * sizeof(Branch *));
          for (size_t i = 0; i != hamming_value; ++i)
            m_branches[i]->increment_refs();
        }
        new_branches[pos] = new_branch;
        return ICtrie_Node<HASH_VALUE_TYPE>::Create(this->get_bmp() | flag, hamming_value + 1, new_branches);
      }

      const ICtrie_Node<HASH_VALUE_TYPE> * updated(const size_t pos, const HASH_VALUE_TYPE flag, Branch * const new_branch) const override {
        assert(get_bmp() & flag);
        std::array<Branch *, hamming<HASH_VALUE_TYPE>()> new_branches;
        if (hamming_value) {
          std::memcpy(new_branches.data(), m_branches.data(), pos * sizeof(Branch *));
          std::memcpy(new_branches.data() + (pos + 1), m_branches.data() + (pos + 1), (hamming_value - pos - 1) * sizeof(Branch *));
          for (size_t i = 0; i != pos; ++i)
            new_branches[i]->increment_refs();
          for (size_t i = pos + 1; i != hamming_value; ++i)
            new_branches[i]->increment_refs();
        }
        new_branches[pos] = new_branch;
        return ICtrie_Node<HASH_VALUE_TYPE>::Create(this->get_bmp(), hamming_value, new_branches);
      }

      const ICtrie_Node<HASH_VALUE_TYPE> * removed(const size_t pos, const HASH_VALUE_TYPE flag) const override {
        assert(get_bmp() & flag);
        std::array<Branch *, hamming<HASH_VALUE_TYPE>()> new_branches;
        if (hamming_value) {
          std::memcpy(new_branches.data(), m_branches.data(), pos * sizeof(Branch *));
          std::memcpy(new_branches.data() + pos, m_branches.data() + (pos + 1), (hamming_value - pos - 1) * sizeof(Branch *));
          for (size_t i = 0; i != hamming_value - 1; ++i)
            new_branches[i]->increment_refs();
        }
        return ICtrie_Node<HASH_VALUE_TYPE>::Create(this->get_bmp() & ~flag, hamming_value - 1, new_branches);
      }

    private:
      const std::array<Branch *, hamming_value> m_branches;
    };

    template <typename HASH_VALUE_TYPE, size_t IN = hamming<HASH_VALUE_TYPE>()>
    struct Ctrie_Node_Generator {
      static void Create(std::array<std::function<const ICtrie_Node<HASH_VALUE_TYPE> *(const HASH_VALUE_TYPE, const std::array<Branch *, hamming<HASH_VALUE_TYPE>()> &)>, sum(hamming<HASH_VALUE_TYPE>(), HASH_VALUE_TYPE(1))> &generator) {
        Ctrie_Node_Generator<HASH_VALUE_TYPE, IN - 1>::Create(generator);

        generator[IN] = [](const HASH_VALUE_TYPE bmp, const std::array<Branch *, hamming<HASH_VALUE_TYPE>()> &branches)->ICtrie_Node<HASH_VALUE_TYPE> * {
          return new Ctrie_Node<HASH_VALUE_TYPE, IN>(bmp, branches);
        };
      }
    };

    template <typename HASH_VALUE_TYPE>
    struct Ctrie_Node_Generator<HASH_VALUE_TYPE, 0> {
      static void Create(std::array<std::function<const ICtrie_Node<HASH_VALUE_TYPE> *(const HASH_VALUE_TYPE, const std::array<Branch *, hamming<HASH_VALUE_TYPE>()> &)>, sum(hamming<HASH_VALUE_TYPE>(), HASH_VALUE_TYPE(1))> &generator) {
        generator[0] = [](const HASH_VALUE_TYPE bmp, const std::array<Branch *, hamming<HASH_VALUE_TYPE>()> &branches)->ICtrie_Node<HASH_VALUE_TYPE> * {
          return new Ctrie_Node<HASH_VALUE_TYPE, 0>(bmp, branches);
        };
      }
    };

    template <typename HASH_VALUE_TYPE>
    ICtrie_Node<HASH_VALUE_TYPE>::Factory::Factory() {
      Ctrie_Node_Generator<HASH_VALUE_TYPE>::Create(m_generator);
    }

    template <typename KEY, typename TYPE>
    struct Singleton_Node : public Branch {
    private:
      Singleton_Node(const Singleton_Node &) = delete;
      Singleton_Node & operator=(const Singleton_Node &) = delete;

    public:
      Singleton_Node() : key(KEY()), value(TYPE()) {}

      template <typename KEY_TYPE, typename TYPE_TYPE>
      Singleton_Node(KEY_TYPE &&key_, TYPE_TYPE &&value_) : key(std::forward<KEY_TYPE>(key_)), value(std::forward<TYPE_TYPE>(value_)) {}

      Branch * resurrect() override {
        increment_refs();
        return this;
      }

      const KEY key;
      const TYPE value;
    };

    template <typename KEY, typename TYPE>
    struct List_Node : public Main_Node {
    private:
      List_Node(const List_Node &) = delete;
      List_Node & operator=(const List_Node &) = delete;

    public:
      List_Node(Singleton_Node<KEY, TYPE> * const snode_, List_Node * const next_ = nullptr) : snode(snode_), next(next_) {}

      ~List_Node() {
        snode->decrement_refs();
        while (next && next->decrement_refs()) {
          List_Node * next_next = next->next;
          next->next = nullptr;
          next = next_next;
        }
      }

      const List_Node * inserted(Singleton_Node<KEY, TYPE> * const snode) const {
        List_Node * new_head = nullptr;
        auto old_head = this;
        for (; old_head; old_head = old_head->next) {
          if (old_head->snode->key == snode->key) {
            old_head = old_head->next;
            break;
          }
          else {
            old_head->snode->increment_refs();
            new_head = new List_Node(old_head->snode, new_head);
          }
        }
        for (; old_head; old_head = old_head->next) {
          old_head->snode->increment_refs();
          new_head = new List_Node(old_head->snode, new_head);
        }
        return new List_Node(snode, new_head);
      }

      std::pair<List_Node *, const Singleton_Node<KEY, TYPE> *> removed(const KEY &key) const {
        List_Node * new_head = nullptr;
        const Singleton_Node<KEY, TYPE> * new_found = nullptr;
        auto old_head = this;
        for (; old_head; old_head = old_head->next) {
          if (old_head->snode->key == key) {
            new_found = old_head->snode;
            old_head = old_head->next;
            break;
          }
          else {
            old_head->snode->increment_refs();
            new_head = new List_Node(old_head->snode, new_head);
          }
        }
        for (; old_head; old_head = old_head->next) {
          old_head->snode->increment_refs();
          new_head = new List_Node(old_head->snode, new_head);
        }
        return std::make_pair(new_head, new_found);
      }

      Singleton_Node<KEY, TYPE> * const snode;
      List_Node * next = nullptr;
    };
  }

  template <typename KEY, typename TYPE, typename HASH = std::hash<KEY>, typename GENERATION = Ctrie_Internal::Generation>
  class Ctrie {
    Ctrie(const Ctrie &) = delete;
    Ctrie & operator=(const Ctrie &) = delete;

    enum class Lookup_Status { FOUND, NOTFOUND, RESTART };
    enum class Insert_Status { OK, RESTART };
    enum class Remove_Status { FOUND, NOTFOUND, RESTART };

  public:
    typedef KEY Key;
    typedef TYPE Type;
    typedef HASH Hash;
    typedef GENERATION Generation;

    typedef decltype(Hash()(Key())) Hash_Value;

    typedef Ctrie_Internal::Main_Node MNode;
    typedef Ctrie_Internal::Branch Branch;
    typedef Ctrie_Internal::ICtrie_Node<Hash_Value> CNode;
    typedef Ctrie_Internal::Singleton_Node<Key, Type> SNode;
    typedef Ctrie_Internal::Tomb_Node TNode;
    typedef Ctrie_Internal::List_Node<Key, Type> LNode;
    typedef Ctrie_Internal::Indirection_Node INode;

    Ctrie() = default;

    ~Ctrie() {
      m_root.load(std::memory_order_acquire)->decrement_refs();
    }

    Intrusive_Shared_Ptr<const SNode> lookup(const Key &key) {
      const Hash_Value hash_value = Hash()(key);
      const SNode * found = nullptr;
      for (;;) {
        INode * root = m_root.load(std::memory_order_acquire);
        if (ilookup(found, root, key, hash_value, 0, nullptr) != Lookup_Status::RESTART)
          break;
      }
      if (found)
        found->increment_refs();
      return found;
    }

    void insert(const Key &key, const Type &value) {
      const Hash_Value hash_value = Hash()(key);
      for (;;) {
        INode * root = m_root.load(std::memory_order_acquire);
        if (iinsert(root, key, hash_value, value, 0, nullptr) != Insert_Status::RESTART)
          break;
      }
    }

    Intrusive_Shared_Ptr<const SNode> remove(const Key &key) {
      const Hash_Value hash_value = Hash()(key);
      const SNode * found = nullptr;
      for (;;) {
        INode * root = m_root.load(std::memory_order_acquire);
        if (iremove(found, root, key, hash_value, 0, nullptr) != Remove_Status::RESTART)
          break;
      }
      if (found)
        found->increment_refs();
      return found;
    }

  private:
    Lookup_Status ilookup(const SNode * &found,
      INode * inode,
      const Key &key,
      const Hash_Value &hash_value,
      size_t level,
      INode * parent) const
    {
      for (;;) {
        const auto inode_main = inode->main.load(std::memory_order_acquire);
        if (const auto cnode = dynamic_cast<const CNode *>(inode_main)) {
          const auto[flag, pos] = cnode->flagpos(hash_value, level);
          Branch * const branch = cnode->get_bmp() & flag ? cnode->at(pos) : nullptr;
          if (!branch)
            return Lookup_Status::NOTFOUND;
          if (const auto inode_next = dynamic_cast<INode *>(branch)) {
            parent = inode;
            inode = inode_next;
            level += CNode::W;
            continue;
          }
          else if (const auto snode = dynamic_cast<const SNode *>(branch)) {
            if (snode->key != key)
              return Lookup_Status::NOTFOUND;
            else {
              found = snode;
              return Lookup_Status::FOUND;
            }
          }
          else
            abort();
        }
        else if (const auto tnode = dynamic_cast<const TNode *>(inode_main)) {
          clean(parent, level - CNode::W);
          return Lookup_Status::RESTART;
        }
        else if (auto lnode = dynamic_cast<const LNode *>(inode_main)) {
          do {
            if (lnode->snode->key == key) {
              found = lnode->snode;
              return Lookup_Status::FOUND;
            }
            else
              lnode = lnode->next;
          } while (lnode);
          return Lookup_Status::NOTFOUND;
        }
        else
          abort();
      }
    }

    Insert_Status iinsert(
      INode * inode,
      const Key &key,
      const Hash_Value &hash_value,
      const Type &value,
      size_t level,
      INode * parent)
    {
      for (;;) {
        auto inode_main = inode->main.load(std::memory_order_acquire);
        if (const auto cnode = dynamic_cast<const CNode *>(inode_main)) {
          const auto[flag, pos] = cnode->flagpos(hash_value, level);
          Branch * branch = cnode->get_bmp() & flag ? cnode->at(pos) : nullptr;
          if (!branch) {
            const auto new_cnode = cnode->inserted(pos, flag, new SNode(key, value));
            return CAS(inode->main, inode_main, new_cnode, std::memory_order_release, std::memory_order_relaxed) ? Insert_Status::OK : Insert_Status::RESTART;
          }
          else {
            if (const auto inode_next = dynamic_cast<INode *>(branch)) {
              parent = inode;
              inode = inode_next;
              level += CNode::W;
              continue;
            }
            else if (const auto snode = dynamic_cast<SNode *>(branch)) {
              const auto new_snode = new SNode(key, value);
              if (snode->key != key) {
                const Hash_Value snode_hash = Hash()(snode->key);
                if (snode_hash != hash_value) {
                  snode->increment_refs();
                  const auto new_cnode = cnode->updated(pos, flag, new INode(CNode::Create(snode, snode_hash, new_snode, hash_value, level + CNode::W), new Generation));
                  return CAS(inode->main, inode_main, new_cnode, std::memory_order_release, std::memory_order_relaxed) ? Insert_Status::OK : Insert_Status::RESTART;
                }
                else {
                  snode->increment_refs();
                  const auto new_cnode = cnode->updated(pos, flag, new INode(new LNode(new_snode, new LNode(snode)), new Generation));
                  return CAS(inode->main, inode_main, new_cnode, std::memory_order_release, std::memory_order_relaxed) ? Insert_Status::OK : Insert_Status::RESTART;
                }
              }
              else {
                const auto new_cnode = cnode->updated(pos, flag, new_snode);
                return CAS(inode->main, inode_main, new_cnode, std::memory_order_release, std::memory_order_relaxed) ? Insert_Status::OK : Insert_Status::RESTART;
              }
            }
            else
              abort();
          }
        }
        else if (const auto tnode = dynamic_cast<const TNode *>(inode_main)) {
          clean(parent, level - CNode::W);
          return Insert_Status::RESTART;
        }
        else if (auto lnode = dynamic_cast<const LNode *>(inode_main)) {
          const Hash_Value lnode_hash = Hash()(lnode->snode->key);
          if (lnode_hash != hash_value) {
            lnode->increment_refs();
            const auto new_cnode = CNode::Create(new SNode(key, value), hash_value, new INode(lnode, new Generation), lnode_hash, level);
            return CAS(inode->main, inode_main, new_cnode, std::memory_order_release, std::memory_order_relaxed) ? Insert_Status::OK : Insert_Status::RESTART;
          }
          else {
            const auto new_lnode = lnode->inserted(new SNode(key, value));
            return CAS(inode->main, inode_main, new_lnode, std::memory_order_release, std::memory_order_relaxed) ? Insert_Status::OK : Insert_Status::RESTART;
          }
        }
      }
    }

    Remove_Status iremove(const SNode * &found,
      INode * inode,
      const Key &key,
      const Hash_Value &hash_value,
      size_t level,
      INode * parent)
    {
      for (;;) {
        auto inode_main = inode->main.load(std::memory_order_acquire);
        if (const auto cnode = dynamic_cast<const CNode *>(inode_main)) {
          const auto[flag, pos] = cnode->flagpos(hash_value, level);
          Branch * branch = cnode->get_bmp() & flag ? cnode->at(pos) : nullptr;
          if (!branch)
            return Remove_Status::NOTFOUND;
          if (const auto inode_next = dynamic_cast<INode *>(branch)) {
            parent = inode;
            inode = inode_next;
            level += CNode::W;
            continue;
          }
          else if (const auto snode = dynamic_cast<const SNode *>(branch)) {
            if (snode->key != key)
              return Remove_Status::NOTFOUND;
            else {
              const CNode * new_cnode = cnode->removed(pos, flag);
              const MNode * new_mainnode = new_cnode->to_contracted(level);
              if (CAS(inode->main, inode_main, new_mainnode, std::memory_order_release, std::memory_order_relaxed)) {
                found = snode;
                if (const auto tnode = dynamic_cast<const TNode *>(new_mainnode))
                  clean_parent(parent, inode, hash_value, level - CNode::W);
                return Remove_Status::FOUND;
              }
              else
                return Remove_Status::RESTART;
            }
          }
          else
            abort();
        }
        else if (const auto tnode = dynamic_cast<const TNode *>(inode_main)) {
          clean(parent, level - CNode::W);
          return Remove_Status::RESTART;
        }
        else if (auto lnode = dynamic_cast<const LNode *>(inode_main)) {
          const auto[new_lnode, new_found] = lnode->removed(key);
          if (!new_found) {
            new_lnode->decrement_refs();
            return Remove_Status::NOTFOUND;
          }
          MNode * new_mainnode = new_lnode;
          if (!new_lnode->next) {
            new_lnode->snode->increment_refs();
            new_mainnode = new TNode(new_lnode->snode);
            delete new_lnode;
          }
          if (CAS(inode->main, inode_main, new_mainnode, std::memory_order_release, std::memory_order_relaxed)) {
            found = new_found;
            return Remove_Status::FOUND;
          }
          else
            return Remove_Status::RESTART;
        }
        else
          abort();
      }
    }

    void clean(INode * const inode, const size_t level) const {
      const MNode * inode_main = inode->main.load(std::memory_order_acquire);
      if (auto cnode = dynamic_cast<const CNode *>(inode_main))
        CAS(inode->main, inode_main, cnode->to_compressed(level), std::memory_order_release, std::memory_order_relaxed);
    }

    void clean_parent(INode * const parent, INode * const inode, const Hash_Value &hash_value, const size_t level) {
      do {
        const MNode * parent_main = parent->main.load(std::memory_order_acquire);
        if (const CNode * const cnode = dynamic_cast<const CNode *>(parent_main)) {
          const auto[flag, pos] = cnode->flagpos(hash_value, level);
          const Branch * const branch = cnode->get_bmp() & flag ? cnode->at(pos) : nullptr;
          if (branch == inode) {
            const MNode * const inode_main = inode->main.load(std::memory_order_acquire);
            if (const TNode * const tnode = dynamic_cast<const TNode *>(inode_main)) {
              const auto new_cnode = cnode->updated(pos, flag, inode->resurrect())->to_contracted(level);
              if (!CAS(parent->main, parent_main, new_cnode, std::memory_order_release, std::memory_order_relaxed))
                continue;
            }
          }
        }
      } while (false);
    }

    template <typename VALUE_TYPE, typename DESIRED>
    static bool CAS(std::atomic<VALUE_TYPE *> &atomic_value, VALUE_TYPE * &expected, const DESIRED * desired, const std::memory_order success = std::memory_order_seq_cst, const std::memory_order failure = std::memory_order_seq_cst) {
      if (atomic_value.compare_exchange_strong(expected, desired, success, failure)) {
        expected->decrement_refs();
        return true;
      }
      else {
        desired->decrement_refs();
        return false;
      }
    }

    ZENI_CONCURRENCY_CACHE_ALIGN std::atomic<INode *> m_root = new INode(CNode::Create(0x0, 0, {}), new Generation);
  };

}

#endif
