#ifndef ZENI_CONCURRENCY_CTRIE_NS_HPP
#define ZENI_CONCURRENCY_CTRIE_NS_HPP

#include "Intrusive_Shared_Ptr.hpp"
#include "../../Internal/Mallocator.hpp"

#include <array>
#include <cstring>
#include <functional>
#include <stack>

namespace Zeni::Concurrency {

  template <typename KEY, typename HASH = std::hash<KEY>, typename PRED = std::equal_to<KEY>, typename FLAG_TYPE = uint32_t, typename ALLOCATOR = Jemallocator<char>>
  class Ctrie_NS;

  namespace Ctrie_NS_Internal {

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

    template <typename ALLOCATOR>
    struct Early_Decrement_Propagation : public Enable_Intrusive_Sharing, public Allocated<typename ALLOCATOR::template Aligned<std::align_val_t(ZENI_CONCURRENCY_DESTRUCTIVE_INTERFERENCE_SIZE)>::Allocator> {
      void immediate_deletion() const {
        const_cast<Early_Decrement_Propagation *>(this)->on_final_decrement();
        delete this;
      }

      int64_t decrement_refs() const {
        const int64_t refs = Enable_Intrusive_Sharing::decrement_refs();
        if (refs == 1)
          const_cast<Early_Decrement_Propagation *>(this)->on_final_decrement();
        return refs;
      }

      virtual void on_final_decrement() {}
    };

    template <typename ALLOCATOR>
    struct Node : public Early_Decrement_Propagation<ALLOCATOR> {
    private:
      Node(const Node &) = delete;
      Node & operator=(const Node &) = delete;

    public:
      Node() = default;
    };

    template <typename ALLOCATOR>
    struct Main_Node : public Node<ALLOCATOR> {
    private:
      Main_Node(const Main_Node &) = delete;
      Main_Node & operator=(const Main_Node &) = delete;

    public:
      Main_Node() = default;
    };

    template <typename ALLOCATOR>
    struct Branch : public Node<ALLOCATOR> {
    private:
      Branch(const Branch &) = delete;
      Branch & operator=(const Branch &) = delete;

    public:
      Branch() = default;

      virtual Branch * resurrect() {
        abort();
      }
    };

    template <typename ALLOCATOR>
    struct Tomb_Node : public Main_Node<ALLOCATOR> {
    private:
      Tomb_Node(const Tomb_Node &) = delete;
      Tomb_Node & operator=(const Tomb_Node &) = delete;

    public:
      Tomb_Node() = default;

      Tomb_Node(Branch<ALLOCATOR> * const branch_) : branch(branch_) {}

      void on_final_decrement() override {
        Main_Node<ALLOCATOR>::on_final_decrement();
        branch->decrement_refs();
      }

      Branch<ALLOCATOR> * const branch;
    };

    template <typename ALLOCATOR>
    struct Indirection_Node : public Branch<ALLOCATOR> {
    private:
      Indirection_Node(const Indirection_Node &) = delete;
      Indirection_Node & operator=(const Indirection_Node &) = delete;

    public:
      Indirection_Node(const Main_Node<ALLOCATOR> * const main_)
        : main(main_)
      {
      }

      void on_final_decrement() override {
        const Main_Node<ALLOCATOR> * const m = ptr_part(main.load(std::memory_order_acquire));
        if (m)
          m->decrement_refs();
      }

      static bool is_marked_readonly(const Main_Node<ALLOCATOR> * const ptr) {
        return reinterpret_cast<const uintptr_t>(ptr) & 0x1;
      }
      static const Main_Node<ALLOCATOR> * ptr_part(const Main_Node<ALLOCATOR> * const ptr) {
        return reinterpret_cast<const Main_Node<ALLOCATOR> *>(reinterpret_cast<const uintptr_t>(ptr) & ~0x3);
      }
      static const Main_Node<ALLOCATOR> * mark_readonly(const Main_Node<ALLOCATOR> * const ptr) {
        const uintptr_t ival = reinterpret_cast<const uintptr_t>(ptr) & ~0x3;
        return ival & 0x3 ? ptr : reinterpret_cast<const Main_Node<ALLOCATOR> *>(reinterpret_cast<const uintptr_t>(ptr) | 0x1);
      }

      Branch<ALLOCATOR> * resurrect() override {
        if (const auto tomb_node = dynamic_cast<const Tomb_Node<ALLOCATOR> *>(main.load(std::memory_order_acquire)))
          return tomb_node->branch->try_increment_refs() ? tomb_node->branch : nullptr;
        else
          return this->try_increment_refs() ? this : nullptr;
      }

      ZENI_CONCURRENCY_CACHE_ALIGN std::atomic<const Main_Node<ALLOCATOR> *> main;
    };

    template <typename KEY, typename ALLOCATOR>
    struct Singleton_Node : public Branch<ALLOCATOR> {
    private:
      Singleton_Node(const Singleton_Node &) = delete;
      Singleton_Node & operator=(const Singleton_Node &) = delete;

    public:
      template <typename KEY_TYPE>
      Singleton_Node(KEY_TYPE &&key_) : key(std::forward<KEY_TYPE>(key_)) {}

      Branch<ALLOCATOR> * resurrect() override {
        return this->try_increment_refs() ? this : nullptr;
      }

      const KEY key;
    };

    template <typename KEY, typename HASH, typename PRED, typename FLAG_TYPE, typename ALLOCATOR>
    struct ICtrie_Node : public Main_Node<ALLOCATOR> {
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
      typedef decltype(HASH()(KEY())) Hash_Value;
      static const FLAG_TYPE W = log2(hamming_max);

      static const ICtrie_Node * Create(const FLAG_TYPE bmp, const size_t hamming_value, const std::array<Branch<ALLOCATOR> *, hamming_max> &branches) {
        static Factory factory;
        return factory.create(bmp, hamming_value, branches);
      }

      static const ICtrie_Node * Create(Branch<ALLOCATOR> * const first, const Hash_Value first_hash, Branch<ALLOCATOR> * const second, const Hash_Value second_hash, const size_t level) {
        assert(first_hash != second_hash);
        const auto first_flag = flag(first_hash, level);
        const auto second_flag = flag(second_hash, level);
        if (first_flag == second_flag) {
          std::array<Branch<ALLOCATOR> *, hamming_max> branches;
          branches[0] = new Indirection_Node<ALLOCATOR>(Create(first, first_hash, second, second_hash, level + W));
          return Create(first_flag, 1, branches);
        }
        else {
          std::array<Branch<ALLOCATOR> *, hamming_max> branches;
          branches[0] = first_flag < second_flag ? first : second;
          branches[1] = first_flag > second_flag ? first : second;
          return Create(first_flag | second_flag, 2, branches);
        }
      }

      FLAG_TYPE get_bmp() const { return m_bmp; }

      virtual size_t get_hamming_value() const = 0;

      std::pair<FLAG_TYPE, size_t> flagpos(const Hash_Value hash_value, const size_t level) const {
        const FLAG_TYPE desired_bit = flag(hash_value, level);
        const size_t array_index = hamming(m_bmp & (desired_bit - 1));
        return std::make_pair(desired_bit, array_index);
      }

      virtual Branch<ALLOCATOR> * at(const size_t i) const = 0;

      virtual const ICtrie_Node * inserted(const size_t pos, const FLAG_TYPE flag, Branch<ALLOCATOR> * const new_branch) const = 0;

      virtual const ICtrie_Node * updated(const size_t pos, const FLAG_TYPE flag, Branch<ALLOCATOR> * const new_branch) const = 0;

      virtual const ICtrie_Node * erased(const size_t pos, const FLAG_TYPE flag) const = 0;

      const Main_Node<ALLOCATOR> * to_compressed(const size_t level) const {
        bool at_least_one = false;
        std::array<Branch<ALLOCATOR> *, hamming_max> branches;
        for (size_t i = 0; i != get_hamming_value(); ++i) {
          Branch<ALLOCATOR> * const resurrected = at(i)->resurrect();
          if (!resurrected) {
            for (size_t j = 0; j != i; ++j)
              branches[j]->decrement_refs();
            return nullptr;
          }
          branches[i] = resurrected;
          at_least_one |= resurrected != at(i);
        }
        if (at_least_one)
          return Create(m_bmp, get_hamming_value(), branches)->to_contracted(level);
        else {
          for (size_t i = 0; i != get_hamming_value(); ++i)
            branches[i]->decrement_refs();
          return nullptr;
        }
      }

      const Main_Node<ALLOCATOR> * to_contracted(const size_t level) const {
        if (level && get_hamming_value() == 1) {
          Branch<ALLOCATOR> * const branch = at(0);
          [[maybe_unused]] const int64_t refs = branch->try_increment_refs();
          assert(refs);
          this->immediate_deletion();
          return new Tomb_Node<ALLOCATOR>(branch);
        }
        else
          return this;
      }

    private:
      static const Hash_Value unhamming_filter = Hash_Value(unhamming(W));
      FLAG_TYPE m_bmp;

      class Factory {
        Factory(const Factory &) = delete;
        Factory & operator=(const Factory &) = delete;

      public:
        inline Factory();

        const ICtrie_Node<KEY, HASH, PRED, FLAG_TYPE, ALLOCATOR> * create(const FLAG_TYPE bmp, const size_t hamming_value, const std::array<Branch<ALLOCATOR> *, hamming_max> &branches) {
          return m_generator[hamming_value](bmp, branches);
        }

      private:
        std::array<std::function<const ICtrie_Node<KEY, HASH, PRED, FLAG_TYPE, ALLOCATOR> *(const FLAG_TYPE, const std::array<Branch<ALLOCATOR> *, hamming_max> &)>, sum(hamming_max, FLAG_TYPE(1))> m_generator;
      };

      static FLAG_TYPE flag(const Hash_Value hash_value, const size_t level) {
        const Hash_Value shifted_hash = hash_value >> level;
        const Hash_Value desired_bit_index = shifted_hash & unhamming_filter;
        const FLAG_TYPE desired_bit = FLAG_TYPE(1u) << desired_bit_index;
        return desired_bit;
      }
    };

    template <typename KEY, typename HASH, typename PRED, typename FLAG_TYPE, typename ALLOCATOR, size_t HAMMING_VALUE>
    struct Ctrie_Node : public ICtrie_Node<KEY, HASH, PRED, FLAG_TYPE, ALLOCATOR> {
    private:
      Ctrie_Node(const Ctrie_Node &) = delete;
      Ctrie_Node & operator=(const Ctrie_Node &) = delete;

    public:
      typedef decltype(HASH()(KEY())) Hash_Value;

      static const size_t hamming_value = HAMMING_VALUE;

      Ctrie_Node(const FLAG_TYPE bmp, const std::array<Branch<ALLOCATOR> *, hamming<FLAG_TYPE>()> &branches)
        : ICtrie_Node<KEY, HASH, PRED, FLAG_TYPE, ALLOCATOR>(bmp),
        m_branches(reinterpret_cast<const std::array<Branch<ALLOCATOR> *, hamming_value> &>(branches)) //< Should always be smaller, safe to copy subset
      {
      }

      ~Ctrie_Node() {
        for (auto branch : m_branches)
          branch->decrement_refs();
      }

      void on_final_decrement() override {
        Main_Node<ALLOCATOR>::on_final_decrement();
        //for (auto branch : m_branches)
        //  branch->decrement_refs();
      }

      size_t get_hamming_value() const override {
        return hamming_value;
      };

      Branch<ALLOCATOR> * at(const size_t i) const override {
        assert(i >= 0 && i < m_branches.size());
        return m_branches[i];
      }

      const ICtrie_Node<KEY, HASH, PRED, FLAG_TYPE, ALLOCATOR> * inserted(const size_t pos, const FLAG_TYPE flag, Branch<ALLOCATOR> * const new_branch) const override {
        assert(!(this->get_bmp() & flag));
        std::array<Branch<ALLOCATOR> *, hamming<FLAG_TYPE>()> new_branches;
        if (hamming_value) {
          std::memcpy(new_branches.data(), m_branches.data(), pos * sizeof(Branch<ALLOCATOR> *));
          std::memcpy(new_branches.data() + (pos + 1), m_branches.data() + pos, (hamming_value - pos) * sizeof(Branch<ALLOCATOR> *));
          for (size_t i = 0; i != hamming_value; ++i) {
            if (!m_branches[i]->try_increment_refs()) {
              for (size_t j = 0; j != i; ++j)
                m_branches[j]->decrement_refs();
              new_branch->decrement_refs();
              return nullptr;
            }
          }
        }
        new_branches[pos] = new_branch;
        return ICtrie_Node<KEY, HASH, PRED, FLAG_TYPE, ALLOCATOR>::Create(this->get_bmp() | flag, hamming_value + 1, new_branches);
      }

      const ICtrie_Node<KEY, HASH, PRED, FLAG_TYPE, ALLOCATOR> * updated(const size_t pos, const FLAG_TYPE flag, Branch<ALLOCATOR> * const new_branch) const override {
        assert(this->get_bmp() & flag);
        std::array<Branch<ALLOCATOR> *, hamming<FLAG_TYPE>()> new_branches;
        if (hamming_value) {
          std::memcpy(new_branches.data(), m_branches.data(), pos * sizeof(Branch<ALLOCATOR> *));
          std::memcpy(new_branches.data() + (pos + 1), m_branches.data() + (pos + 1), (hamming_value - pos - 1) * sizeof(Branch<ALLOCATOR> *));
          for (size_t i = 0; i != pos; ++i) {
            if (!new_branches[i]->try_increment_refs()) {
              for (size_t j = 0; j != i; ++j)
                new_branches[j]->decrement_refs();
              new_branch->decrement_refs();
              return nullptr;
            }
          }
          for (size_t i = pos + 1; i != hamming_value; ++i) {
            if (!new_branches[i]->try_increment_refs()) {
              for (size_t j = 0; j != pos; ++j)
                new_branches[j]->decrement_refs();
              for (size_t j = pos + 1; j != i; ++j)
                new_branches[j]->decrement_refs();
              new_branch->decrement_refs();
              return nullptr;
            }
          }
        }
        new_branches[pos] = new_branch;
        return ICtrie_Node<KEY, HASH, PRED, FLAG_TYPE, ALLOCATOR>::Create(this->get_bmp(), hamming_value, new_branches);
      }

      const ICtrie_Node<KEY, HASH, PRED, FLAG_TYPE, ALLOCATOR> * erased(const size_t pos, const FLAG_TYPE flag) const override {
        assert(this->get_bmp() & flag);
        std::array<Branch<ALLOCATOR> *, hamming<FLAG_TYPE>()> new_branches;
        if (hamming_value) {
          std::memcpy(new_branches.data(), m_branches.data(), pos * sizeof(Branch<ALLOCATOR> *));
          std::memcpy(new_branches.data() + pos, m_branches.data() + (pos + 1), (hamming_value - pos - 1) * sizeof(Branch<ALLOCATOR> *));
          for (size_t i = 0; i != hamming_value - 1; ++i) {
            if (!new_branches[i]->try_increment_refs()) {
              for (size_t j = 0; j != i; ++j)
                new_branches[j]->decrement_refs();
              return nullptr;
            }
          }
        }
        return ICtrie_Node<KEY, HASH, PRED, FLAG_TYPE, ALLOCATOR>::Create(this->get_bmp() & ~flag, hamming_value - 1, new_branches);
      }

    private:
      const std::array<Branch<ALLOCATOR> *, hamming_value> m_branches;
    };

    template <typename KEY, typename HASH, typename PRED, typename FLAG_TYPE, typename ALLOCATOR, size_t IN = hamming<FLAG_TYPE>()>
    struct Ctrie_Node_Generator {
      static void Create(std::array<std::function<const ICtrie_Node<KEY, HASH, PRED, FLAG_TYPE, ALLOCATOR> *(const FLAG_TYPE, const std::array<Branch<ALLOCATOR> *, hamming<FLAG_TYPE>()> &)>, sum(hamming<FLAG_TYPE>(), FLAG_TYPE(1))> &generator) {
        Ctrie_Node_Generator<KEY, HASH, PRED, FLAG_TYPE, ALLOCATOR, IN - 1>::Create(generator);

        generator[IN] = [](const FLAG_TYPE bmp, const std::array<Branch<ALLOCATOR> *, hamming<FLAG_TYPE>()> &branches)->ICtrie_Node<KEY, HASH, PRED, FLAG_TYPE, ALLOCATOR> * {
          return new Ctrie_Node<KEY, HASH, PRED, FLAG_TYPE, ALLOCATOR, IN>(bmp, branches);
        };
      }
    };

    template <typename KEY, typename HASH, typename PRED, typename FLAG_TYPE, typename ALLOCATOR>
    struct Ctrie_Node_Generator<KEY, HASH, PRED, FLAG_TYPE, ALLOCATOR, 0> {
      static void Create(std::array<std::function<const ICtrie_Node<KEY, HASH, PRED, FLAG_TYPE, ALLOCATOR> *(const FLAG_TYPE, const std::array<Branch<ALLOCATOR> *, hamming<FLAG_TYPE>()> &)>, sum(hamming<FLAG_TYPE>(), FLAG_TYPE(1))> &generator) {
        generator[0] = [](const FLAG_TYPE bmp, const std::array<Branch<ALLOCATOR> *, hamming<FLAG_TYPE>()> &branches)->ICtrie_Node<KEY, HASH, PRED, FLAG_TYPE, ALLOCATOR> * {
          return new Ctrie_Node<KEY, HASH, PRED, FLAG_TYPE, ALLOCATOR, 0>(bmp, branches);
        };
      }
    };

    template <typename KEY, typename HASH, typename PRED, typename FLAG_TYPE, typename ALLOCATOR>
    ICtrie_Node<KEY, HASH, PRED, FLAG_TYPE, ALLOCATOR>::Factory::Factory() {
      Ctrie_Node_Generator<KEY, HASH, PRED, FLAG_TYPE, ALLOCATOR>::Create(m_generator);
    }

    template <typename KEY, typename PRED, typename ALLOCATOR>
    struct List_Node : public Main_Node<ALLOCATOR> {
    private:
      List_Node(const List_Node &) = delete;
      List_Node & operator=(const List_Node &) = delete;

    public:
      enum class Result {
        Restart,             ///< Ctrie_NS operation must restart due to concurrent activity
        Found,               ///< Lookup successful
        Not_Found,           ///< Lookup failed
        First_Insertion,     ///< Count increases to 1 and object inserted into trie
        Last_Removal,        ///< Count decrements to 0 and object removed from trie
        Replacing_Insertion, ///< Object inserted into trie replaces previous object associated with given key value
        Failed_Removal       ///< Object was not present in the trie and could not be removed
      };

      List_Node(Singleton_Node<KEY, ALLOCATOR> * const snode_, List_Node * const next_ = nullptr) : snode(snode_), next(next_) {}

      ~List_Node() {
        while (next) {
          if (next->decrement_refs() > 1)
            break;
          List_Node * next_next = next->next;
          next->next = nullptr;
          next = next_next;
        }
      }

      void on_final_decrement() override {
        Main_Node<ALLOCATOR>::on_final_decrement();
        snode->decrement_refs();
      }

      std::tuple<Result, List_Node *, const Singleton_Node<KEY, ALLOCATOR> *, const Singleton_Node<KEY, ALLOCATOR> *> updated(const KEY &key, const bool insertion) const {
        const Singleton_Node<KEY, ALLOCATOR> * found = nullptr;
        List_Node * new_head = nullptr;
        List_Node * new_tail = nullptr;
        List_Node * old_head = const_cast<List_Node *>(this);
        for (; old_head; old_head = old_head->next) {
          if (PRED()(old_head->snode->key, key)) {
            found = old_head->snode;
            if (old_head->next) {
              if (!old_head->next->try_increment_refs()) {
                if (new_head)
                  new_head->immediate_deletion();
                return std::make_tuple(Result::Restart, nullptr, nullptr, nullptr);
              }
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
            if (!old_head->snode->try_increment_refs()) {
              if (new_head)
                new_head->immediate_deletion();
              return std::make_tuple(Result::Restart, nullptr, nullptr, nullptr);
            }
            if (new_head)
              new_head = new List_Node(old_head->snode, new_head);
            else {
              new_head = new List_Node(old_head->snode, nullptr);
              new_tail = new_head;
            }
          }
        }
        if (found) {
          if (insertion) {
            const auto new_snode = new Singleton_Node<KEY, ALLOCATOR>(key);
            return std::make_tuple(Result::Replacing_Insertion, new List_Node(new_snode, new_head), new_snode, found);
          }
          else
            return std::make_tuple(Result::Last_Removal, new_head, found, nullptr);
        }
        else {
          if (insertion) {
            const auto new_snode = new Singleton_Node<KEY, ALLOCATOR>(key);
            return std::make_tuple(Result::First_Insertion, new List_Node(new_snode, new_head), new_snode, nullptr);
          }
          else
            return std::make_tuple(Result::Failed_Removal, new_head, nullptr, nullptr);
        }
      }

      Singleton_Node<KEY, ALLOCATOR> * const snode;
      List_Node * next = nullptr;
    };
  }

  template <typename KEY, typename HASH, typename PRED, typename FLAG_TYPE, typename ALLOCATOR>
  class Ctrie_NS : public Allocated<typename ALLOCATOR::template Aligned<std::align_val_t(ZENI_CONCURRENCY_DESTRUCTIVE_INTERFERENCE_SIZE)>::Allocator> {
    Ctrie_NS(const Ctrie_NS &) = delete;
    Ctrie_NS operator=(const Ctrie_NS &) = delete;

  public:
    typedef KEY Key;
    typedef HASH Hash;
    typedef PRED Pred;
    typedef ALLOCATOR Allocator;

    typedef decltype(Hash()(Key())) Hash_Value;
    typedef FLAG_TYPE Flag_Type;

    typedef Ctrie_NS_Internal::Node<Allocator> Node;
    typedef Ctrie_NS_Internal::Main_Node<Allocator> MNode;
    typedef Ctrie_NS_Internal::Singleton_Node<Key, Allocator> SNode;
    typedef Ctrie_NS_Internal::ICtrie_Node<Key, Hash, Pred, Flag_Type, Allocator> CNode;
    typedef Ctrie_NS_Internal::List_Node<Key, Pred, Allocator> LNode;
    typedef Ctrie_NS_Internal::Branch<Allocator> Branch;
    typedef Ctrie_NS_Internal::Tomb_Node<Allocator> TNode;
    typedef Ctrie_NS_Internal::Indirection_Node<Allocator> INode;

    typedef typename LNode::Result Result;

    Ctrie_NS() = default;

    ~Ctrie_NS() {
      const Branch * const branch = m_root.load(std::memory_order_acquire);
      if (branch)
        branch->decrement_refs();
    }

    bool empty() const {
      for (;;) {
        INode * const root = Read_Root();
        const auto inode_main = CAS_Read(root);
        if (inode_main) {
          if (const auto cnode = dynamic_cast<const CNode *>(inode_main))
            return cnode->get_hamming_value() == 0;
          else
            return false;
        }
      }
    }

    template <typename Comparable, typename CHash = Hash, typename CPred = Pred>
    std::pair<Result, Key> lookup(const Comparable &key) const {
      const Hash_Value hash_value = CHash()(key);
      for (;;) {
        INode * const root = Read_Root();
        const auto[result, found] = ilookup<Comparable, CPred>(root, key, hash_value, 0, nullptr);
        if (result != Result::Restart)
          return found ? std::make_pair(Result::Found, found->key) : std::make_pair(Result::Not_Found, Key());
      }
    }

    std::tuple<Result, Key, Key> insert(const Key &key) {
      const Hash_Value hash_value = Hash()(key);
      for (;;) {
        INode * const root = Read_Root();
        const auto[result, inserted, replaced] = iinsert(root, key, hash_value, 0, nullptr);
        if (result != Result::Restart)
          return std::make_tuple(result, inserted ? inserted->key : Key(), replaced ? replaced->key : Key());
      }
    }

    std::pair<Result, Key> erase(const Key &key) {
      const Hash_Value hash_value = Hash()(key);
      for (;;) {
        INode * const root = Read_Root();
        const auto[result, removed] = ierase(root, key, hash_value, 0, nullptr);
        if (result != Result::Restart)
          return std::make_pair(result, removed ? removed->key : Key());
      }
    }

    void clear() {
      auto other = new INode(CNode::Create(0x0, 0, {}));
      other = m_root.exchange(other, std::memory_order_acq_rel);
      other->decrement_refs();
    }

  private:
    INode * Read_Root() const {
      return dynamic_cast<INode *>(m_root.load(std::memory_order_acquire));
    }

    template <typename Comparable, typename CPred>
    std::pair<Result, const SNode *> ilookup(
      INode * inode,
      const Comparable &key,
      const Hash_Value &hash_value,
      size_t level,
      INode * parent) const
    {
      for (;;) {
        const auto inode_main = CAS_Read(inode);
        if (!inode_main)
          return std::make_pair(Result::Restart, nullptr);
        if (const auto cnode = dynamic_cast<const CNode *>(inode_main)) {
          const auto[flag, pos] = cnode->flagpos(hash_value, level);
          Branch * const branch = cnode->get_bmp() & flag ? cnode->at(pos) : nullptr;
          if (!branch)
            return std::make_pair(Result::Not_Found, nullptr);
          if (const auto inode_next = dynamic_cast<INode *>(branch)) {
            if (deepen(inode, inode_main, level, parent, cnode, pos, inode_next))
              continue;
            else
              return std::make_pair(Result::Restart, nullptr);
          }
          else if (const auto snode = dynamic_cast<const SNode *>(branch)) {
            if (CPred()(key, snode->key))
              return std::make_pair(Result::Found, snode);
            else
              return std::make_pair(Result::Not_Found, nullptr);
          }
          else
            abort();
        }
        else if (const auto tnode = dynamic_cast<const TNode *>(inode_main)) {
          clean(parent, level - CNode::W);
          return std::make_pair(Result::Restart, nullptr);
        }
        else if (auto lnode = dynamic_cast<const LNode *>(inode_main)) {
          do {
            if (CPred()(lnode->snode->key, key))
              return std::make_pair(Result::Found, lnode->snode);
            else
              lnode = lnode->next;
          } while (lnode);
          return std::make_pair(Result::Not_Found, nullptr);
        }
        else
          abort();
      }
    }

    std::tuple<Result, const SNode *, const SNode *> iinsert(
      INode * inode,
      const Key &key,
      const Hash_Value &hash_value,
      size_t level,
      INode * parent)
    {
      for (;;) {
        const auto inode_main = CAS_Read(inode);
        if (!inode_main)
          return std::make_tuple(Result::Restart, nullptr, nullptr);
        if (const auto cnode = dynamic_cast<const CNode *>(inode_main)) {
          const auto[flag, pos] = cnode->flagpos(hash_value, level);
          Branch * branch = cnode->get_bmp() & flag ? cnode->at(pos) : nullptr;
          if (!branch) {
            const auto new_snode = new SNode(key);
            const auto new_cnode = cnode->inserted(pos, flag, new_snode);
            if (!new_cnode)
              return std::make_tuple(Result::Restart, nullptr, nullptr);
            if (CAS_del(inode, inode_main, new_cnode))
              return std::make_tuple(Result::First_Insertion, new_snode, nullptr);
            else
              return std::make_tuple(Result::Restart, nullptr, nullptr);
          }
          else {
            if (const auto inode_next = dynamic_cast<INode *>(branch)) {
              if (deepen(inode, inode_main, level, parent, cnode, pos, inode_next))
                continue;
              else
                return std::make_tuple(Result::Restart, nullptr, nullptr);
            }
            else if (const auto snode = dynamic_cast<SNode *>(branch)) {
              if (Pred()(snode->key, key)) {
                const auto new_snode = new SNode(key);
                const auto new_cnode = cnode->updated(pos, flag, new_snode);
                if (!new_cnode)
                  return std::make_tuple(Result::Restart, nullptr, nullptr);
                if (CAS_del(inode, inode_main, new_cnode))
                  return std::make_tuple(Result::Replacing_Insertion, new_snode, snode);
                else
                  return std::make_tuple(Result::Restart, nullptr, nullptr);
              }
              else {
                const Hash_Value snode_hash = Hash()(snode->key);
                if (snode_hash != hash_value) {
                  if (!snode->try_increment_refs())
                    return std::make_tuple(Result::Restart, nullptr, nullptr);
                  const auto new_snode = new SNode(key);
                  const auto new_cnode = cnode->updated(pos, flag, new INode(CNode::Create(snode, snode_hash, new_snode, hash_value, level + CNode::W)));
                  if (!new_cnode)
                    return std::make_tuple(Result::Restart, nullptr, nullptr);
                  if (CAS_del(inode, inode_main, new_cnode))
                    return std::make_tuple(Result::First_Insertion, new_snode, nullptr);
                  else
                    return std::make_tuple(Result::Restart, nullptr, nullptr);
                }
                else {
                  if (!snode->try_increment_refs())
                    return std::make_tuple(Result::Restart, nullptr, nullptr);
                  const auto new_snode = new SNode(key);
                  const auto new_cnode = cnode->updated(pos, flag, new INode(new LNode(new_snode, new LNode(snode))));
                  if (!new_cnode)
                    return std::make_tuple(Result::Restart, nullptr, nullptr);
                  if (CAS_del(inode, inode_main, new_cnode))
                    return std::make_tuple(Result::First_Insertion, new_snode, nullptr);
                  else
                    return std::make_tuple(Result::Restart, nullptr, nullptr);
                }
              }
            }
            else
              abort();
          }
        }
        else if (const auto tnode = dynamic_cast<const TNode *>(inode_main)) {
          clean(parent, level - CNode::W);
          return std::make_tuple(Result::Restart, nullptr, nullptr);
        }
        else if (auto lnode = dynamic_cast<const LNode *>(inode_main)) {
          const Hash_Value lnode_hash = Hash()(lnode->snode->key);
          if (lnode_hash != hash_value) {
            if (!lnode->try_increment_refs())
              return std::make_tuple(Result::Restart, nullptr, nullptr);
            const auto new_snode = new SNode(key);
            const auto new_cnode = CNode::Create(new_snode, hash_value, new INode(lnode), lnode_hash, level);
            if (!new_cnode)
              return std::make_tuple(Result::Restart, nullptr, nullptr);
            if (CAS_del(inode, inode_main, new_cnode))
              return std::make_tuple(Result::First_Insertion, new_snode, nullptr);
            else
              return std::make_tuple(Result::Restart, nullptr, nullptr);
          }
          else {
            const auto[result, new_lnode, inserted, replaced] = lnode->updated(key, true);
            if (result == Result::Restart)
              return std::make_tuple(Result::Restart, nullptr, nullptr);
            if (CAS_del(inode, inode_main, new_lnode))
              return std::make_tuple(Result::First_Insertion, inserted, replaced);
            else
              return std::make_tuple(Result::Restart, inserted, replaced);
          }
        }
      }
    }

    std::pair<Result, const SNode *> ierase(
      INode * inode,
      const Key &key,
      const Hash_Value &hash_value,
      size_t level,
      INode * parent)
    {
      for (;;) {
        const auto inode_main = CAS_Read(inode);
        if (!inode_main)
          return std::make_pair(Result::Restart, nullptr);
        if (const auto cnode = dynamic_cast<const CNode *>(inode_main)) {
          const auto[flag, pos] = cnode->flagpos(hash_value, level);
          Branch * branch = cnode->get_bmp() & flag ? cnode->at(pos) : nullptr;
          if (!branch)
            return std::make_pair(Result::Not_Found, nullptr);
          if (const auto inode_next = dynamic_cast<INode *>(branch)) {
            if (deepen(inode, inode_main, level, parent, cnode, pos, inode_next))
              continue;
            else
              return std::make_pair(Result::Restart, nullptr);
          }
          else if (const auto snode = dynamic_cast<const SNode *>(branch)) {
            if (Pred()(key, snode->key)) {
              const CNode * new_cnode = cnode->erased(pos, flag);
              if (!new_cnode)
                return std::make_pair(Result::Restart, nullptr);
              const MNode * new_mainnode = new_cnode->to_contracted(level);
              if (!new_mainnode)
                return std::make_pair(Result::Restart, nullptr);
              if (CAS_del(inode, inode_main, new_mainnode)) {
                if (const auto tnode = dynamic_cast<const TNode *>(new_mainnode))
                  clean_parent(parent, inode, hash_value, level - CNode::W);
                return std::make_pair(Result::Last_Removal, snode);
              }
              else
                return std::make_pair(Result::Restart, nullptr);
            }
            else
              return std::make_pair(Result::Not_Found, nullptr);
          }
          else
            abort();
        }
        else if (const auto tnode = dynamic_cast<const TNode *>(inode_main)) {
          clean(parent, level - CNode::W);
          return std::make_pair(Result::Restart, nullptr);
        }
        else if (auto lnode = dynamic_cast<const LNode *>(inode_main)) {
          const auto[result, new_lnode, removed, unused] = lnode->updated(key, false);
          if (result == Result::Restart)
            return std::make_pair(Result::Restart, nullptr);
          else if (result == Result::Failed_Removal) {
            new_lnode->decrement_refs();
            return std::make_pair(Result::Failed_Removal, nullptr);
          }
          MNode * new_mainnode = new_lnode;
          bool gcas_result;
          if (!new_lnode->next) {
            [[maybe_unused]] const int64_t refs = new_lnode->snode->try_increment_refs();
            assert(refs);
            new_mainnode = new TNode(new_lnode->snode);
            new_lnode->decrement_refs();
            gcas_result = CAS_del(inode, inode_main, new_mainnode);
            if (gcas_result)
              clean_parent(parent, inode, hash_value, level - CNode::W);
          }
          else
            gcas_result = CAS_dec(inode, inode_main, new_mainnode);
          if (gcas_result)
            return std::make_pair(Result::Last_Removal, removed);
          else
            return std::make_pair(Result::Restart, nullptr);
        }
        else
          abort();
      }
    }

    bool deepen(
      INode * &inode,
      const MNode * const inode_main,
      size_t &level,
      INode * &parent,
      const CNode * const cnode,
      const size_t pos,
      INode * const inode_next) const
    {
      parent = inode;
      level += CNode::W;
      inode = inode_next;
      return true;
    }

    void clean(INode * const inode, const size_t level) const {
      const auto inode_main = CAS_Read(inode);
      if (auto cnode = dynamic_cast<const CNode *>(inode_main)) {
        if (const MNode * const compressed = cnode->to_compressed(level))
          CAS_del(inode, inode_main, compressed);
      }
    }

    void clean_parent(INode * const parent, INode * const inode, const Hash_Value &hash_value, const size_t level) {
      do {
        const auto parent_main = CAS_Read(parent);
        if (const CNode * const cnode = dynamic_cast<const CNode *>(parent_main)) {
          const auto[flag, pos] = cnode->flagpos(hash_value, level);
          const Branch * const branch = cnode->get_bmp() & flag ? cnode->at(pos) : nullptr;
          if (branch == inode) {
            const auto inode_main = CAS_Read(inode);
            if (const TNode * const tnode = dynamic_cast<const TNode *>(inode_main)) {
              Branch * const resurrected = inode->resurrect();
              if (!resurrected)
                continue;
              const auto new_cnode = cnode->updated(pos, flag, resurrected)->to_contracted(level);
              if (!new_cnode)
                continue;
              if (!CAS_del(parent, parent_main, new_cnode))
                continue;
            }
          }
        }
      } while (false);
    }

    bool CAS_dec(INode * const inode, const MNode * old_mnode, const MNode * const new_mnode) const {
      return CAS_dec(inode->main, old_mnode, new_mnode, std::memory_order_release, std::memory_order_relaxed);
    }

    bool CAS_del(INode * const inode, const MNode * old_mnode, const MNode * const new_mnode) const {
      return CAS_del(inode->main, old_mnode, new_mnode, std::memory_order_release, std::memory_order_relaxed);
    }

  public:
    const MNode * CAS_Read(const INode * const inode) const {
      return inode->main.load(std::memory_order_acquire);
    }

  private:
    template <typename VALUE_TYPE, typename DESIRED>
    static bool CAS(std::atomic<VALUE_TYPE *> &atomic_value, VALUE_TYPE * &expected, DESIRED * desired, const std::memory_order success = std::memory_order_seq_cst, const std::memory_order failure = std::memory_order_seq_cst) {
      return atomic_value.compare_exchange_strong(expected, desired, success, failure);
    }

    template <typename VALUE_TYPE, typename DESIRED>
    static bool CAS_dec(std::atomic<VALUE_TYPE *> &atomic_value, VALUE_TYPE * &expected, DESIRED * desired, const std::memory_order success = std::memory_order_seq_cst, const std::memory_order failure = std::memory_order_seq_cst) {
      if (atomic_value.compare_exchange_strong(expected, desired, success, failure)) {
        const auto pp_expected = ptr_part(expected);
        if (pp_expected)
          pp_expected->decrement_refs();
        return true;
      }
      else {
        const auto pp_desired = ptr_part(desired);
        if (pp_desired)
          pp_desired->decrement_refs();
        return false;
      }
    }

    template <typename VALUE_TYPE, typename DESIRED>
    static bool CAS_del(std::atomic<VALUE_TYPE *> &atomic_value, VALUE_TYPE * &expected, DESIRED * desired, const std::memory_order success = std::memory_order_seq_cst, const std::memory_order failure = std::memory_order_seq_cst) {
      if (atomic_value.compare_exchange_strong(expected, desired, success, failure)) {
        const auto pp_expected = ptr_part(expected);
        if (pp_expected)
          pp_expected->decrement_refs();
        return true;
      }
      else {
        const auto pp_desired = ptr_part(desired);
        pp_desired->immediate_deletion();
        return false;
      }
    }

    template <typename TYPE>
    static TYPE * ptr_part(TYPE * const ptr) {
      return reinterpret_cast<TYPE *>(reinterpret_cast<uintptr_t>(ptr) & ~0x3);
    }

    ZENI_CONCURRENCY_CACHE_ALIGN mutable std::atomic<Branch *> m_root = new INode(CNode::Create(0x0, 0, {}));
  };

}

#endif
