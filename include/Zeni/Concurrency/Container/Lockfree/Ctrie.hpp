#ifndef ZENI_CONCURRENCY_CTRIE_HPP
#define ZENI_CONCURRENCY_CTRIE_HPP

#include "Intrusive_Shared_Ptr.hpp"
#include "../../Internal/Mallocator.hpp"

#include <array>
#include <cstring>
#include <functional>
#include <stack>

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

    template <typename ALLOCATOR>
    struct Generation : public Enable_Intrusive_Sharing, public Allocated<ALLOCATOR> {
    private:
      Generation(const Generation &) = delete;
      Generation & operator=(const Generation &) = delete;

    public:
      Generation() = default;
    };

    struct Main_or_Failed : public Enable_Intrusive_Sharing {
    private:
      Main_or_Failed(const Main_or_Failed &) = delete;
      Main_or_Failed & operator=(const Main_or_Failed &) = delete;

    public:
      Main_or_Failed() = default;
    };

    struct Main_Node : public Main_or_Failed {
    private:
      Main_Node(const Main_Node &) = delete;
      Main_Node & operator=(const Main_Node &) = delete;

    public:
      Main_Node() = default;

      ~Main_Node() {
        const auto mof = prev.load(std::memory_order_acquire);
        if (mof)
          mof->decrement_refs();
      }

      ZENI_CONCURRENCY_CACHE_ALIGN mutable std::atomic<const Main_or_Failed *> prev = nullptr;
    };

    template <typename ALLOCATOR>
    struct Failed_Node : public Main_or_Failed, public Allocated<typename ALLOCATOR::template Aligned<std::align_val_t(ZENI_CONCURRENCY_DESTRUCTIVE_INTERFERENCE_SIZE)>::Allocator> {
    private:
      Failed_Node(const Failed_Node &) = delete;
      Failed_Node & operator=(const Failed_Node &) = delete;

    public:
      Failed_Node(const Main_Node * const prev_) : prev(prev_) {}

      ~Failed_Node() {
        assert(prev);
        prev->decrement_refs();
      }

      const Main_Node * const prev;
    };

    struct Branch : public Enable_Intrusive_Sharing {
    private:
      Branch(const Branch &) = delete;
      Branch & operator=(const Branch &) = delete;

    public:
      Branch() = default;

      virtual Branch * resurrect() = 0;
    };

    template <typename ALLOCATOR>
    struct Tomb_Node : public Main_Node, public Allocated<typename ALLOCATOR::template Aligned<std::align_val_t(ZENI_CONCURRENCY_DESTRUCTIVE_INTERFERENCE_SIZE)>::Allocator> {
    private:
      Tomb_Node(const Tomb_Node &) = delete;
      Tomb_Node & operator=(const Tomb_Node &) = delete;

    public:
      Tomb_Node() = default;

      Tomb_Node(Branch * const branch_) : branch(branch_) {}

      ~Tomb_Node() {
        branch->decrement_refs();
      }

      Branch * const branch;
    };

    template <typename ALLOCATOR>
    struct Indirection_Node : public Branch, public Allocated<typename ALLOCATOR::template Aligned<std::align_val_t(ZENI_CONCURRENCY_DESTRUCTIVE_INTERFERENCE_SIZE)>::Allocator> {
    private:
      Indirection_Node(const Indirection_Node &) = delete;
      Indirection_Node & operator=(const Indirection_Node &) = delete;

    public:
      Indirection_Node(const Main_Node * main_, const Generation<ALLOCATOR> * generation_)
        : main(main_), generation(generation_)
      {
      }

      ~Indirection_Node() {
        main.load(std::memory_order_acquire)->decrement_refs();
        generation->decrement_refs();
      }

      Branch * resurrect() override {
        if (const auto tomb_node = dynamic_cast<const Tomb_Node<ALLOCATOR> *>(main.load(std::memory_order_acquire)))
          return tomb_node->branch->increment_refs() ? tomb_node->branch : nullptr;
        else
          return increment_refs() ? this : nullptr;
      }

      ZENI_CONCURRENCY_CACHE_ALIGN std::atomic<const Main_Node *> main;
      const Generation<ALLOCATOR> * const generation;
    };

    template <typename ALLOCATOR>
    struct RDCSS_Descriptor : public Main_Node, public Allocated<typename ALLOCATOR::template Aligned<std::align_val_t(ZENI_CONCURRENCY_DESTRUCTIVE_INTERFERENCE_SIZE)>::Allocator> {
      RDCSS_Descriptor(const RDCSS_Descriptor &) = delete;
      RDCSS_Descriptor & operator=(const RDCSS_Descriptor &) = delete;

    public:
      Indirection_Node<ALLOCATOR> * expected_root;
      Main_Node * expected_mnode;
      Main_Node * desired_mnode;
    };

    template <typename KEY, typename ALLOCATOR>
    struct Singleton_Node : public Branch, public Allocated<ALLOCATOR> {
    private:
      Singleton_Node(const Singleton_Node &) = delete;
      Singleton_Node & operator=(const Singleton_Node &) = delete;

    public:
      template <typename KEY_TYPE>
      Singleton_Node(KEY_TYPE &&key_) : key(std::forward<KEY_TYPE>(key_)) {}

      Branch * resurrect() override {
        return increment_refs() ? this : nullptr;
      }

      const KEY key;
    };

    template <typename HASH_VALUE_TYPE, typename FLAG_TYPE, typename ALLOCATOR>
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

      static const ICtrie_Node * Create(const FLAG_TYPE bmp, const size_t hamming_value, const std::array<Branch *, hamming_max> &branches) {
        static Factory factory;
        return factory.create(bmp, hamming_value, branches);
      }

      static const ICtrie_Node * Create(Branch * const first, const HASH_VALUE_TYPE first_hash, Branch * const second, const HASH_VALUE_TYPE second_hash, const size_t level, const Generation<ALLOCATOR> * const generation) {
        assert(first_hash != second_hash);
        assert(level < hamming_max);
        const auto first_flag = flag(first_hash, level);
        const auto second_flag = flag(second_hash, level);
        if (first_flag == second_flag) {
          std::array<Branch *, hamming_max> branches;
          [[maybe_unused]] const int64_t refs = generation->increment_refs();
          assert(refs);
          branches[0] = new Indirection_Node<ALLOCATOR>(Create(first, first_hash, second, second_hash, level + W, generation), generation);
          return Create(first_flag, 1, branches);
        }
        else {
          std::array<Branch *, hamming_max> branches;
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

      virtual Branch * at(const size_t i) const = 0;

      virtual const ICtrie_Node * inserted(const size_t pos, const FLAG_TYPE flag, Branch * const new_branch) const = 0;

      virtual const ICtrie_Node * updated(const size_t pos, const FLAG_TYPE flag, Branch * const new_branch) const = 0;

      virtual const ICtrie_Node * erased(const size_t pos, const FLAG_TYPE flag) const = 0;

      virtual const ICtrie_Node * updated(const Generation<ALLOCATOR> * const new_generation) const = 0;

      const Main_Node * to_compressed(const size_t level) const {
        bool at_least_one = false;
        std::array<Branch *, hamming_max> branches;
        for (size_t i = 0; i != get_hamming_value(); ++i) {
          Branch * const resurrected = at(i)->resurrect();
          if (!resurrected) {
            for (size_t j = 0; j != i; ++j)
              branches[i]->decrement_refs();
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

      const Main_Node * to_contracted(const size_t level) const {
        if (level && get_hamming_value() == 1) {
          Branch * const branch = at(0);
          [[maybe_unused]] const int64_t refs = branch->increment_refs();
          assert(refs);
          delete this;
          return new Tomb_Node<ALLOCATOR>(branch);
        }
        else
          return this;
      }

    private:
      static const HASH_VALUE_TYPE unhamming_filter = HASH_VALUE_TYPE(unhamming(W));
      FLAG_TYPE m_bmp;

      class Factory {
        Factory(const Factory &) = delete;
        Factory & operator=(const Factory &) = delete;

      public:
        inline Factory();

        const ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE, ALLOCATOR> * create(const FLAG_TYPE bmp, const size_t hamming_value, const std::array<Branch *, hamming_max> &branches) {
          return m_generator[hamming_value](bmp, branches);
        }

      private:
        std::array<std::function<const ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE, ALLOCATOR> *(const FLAG_TYPE, const std::array<Branch *, hamming_max> &)>, sum(hamming_max, FLAG_TYPE(1))> m_generator;
      };

      static FLAG_TYPE flag(const HASH_VALUE_TYPE hash_value, const size_t level) {
        const HASH_VALUE_TYPE shifted_hash = hash_value >> level;
        const HASH_VALUE_TYPE desired_bit_index = shifted_hash & unhamming_filter;
        const FLAG_TYPE desired_bit = FLAG_TYPE(1u) << desired_bit_index;
        return desired_bit;
      }
    };

    template <typename HASH_VALUE_TYPE, typename FLAG_TYPE, size_t HAMMING_VALUE, typename ALLOCATOR>
    struct Ctrie_Node : public ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE, ALLOCATOR>, public Allocated<typename ALLOCATOR::template Aligned<std::align_val_t(ZENI_CONCURRENCY_DESTRUCTIVE_INTERFERENCE_SIZE)>::Allocator> {
    private:
      Ctrie_Node(const Ctrie_Node &) = delete;
      Ctrie_Node & operator=(const Ctrie_Node &) = delete;

    public:
      static const size_t hamming_value = HAMMING_VALUE;

      Ctrie_Node(const FLAG_TYPE bmp, const std::array<Branch *, hamming<FLAG_TYPE>()> &branches)
        : ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE, ALLOCATOR>(bmp),
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

      const ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE, ALLOCATOR> * inserted(const size_t pos, const FLAG_TYPE flag, Branch * const new_branch) const override {
        assert(!(this->get_bmp() & flag));
        std::array<Branch *, hamming<FLAG_TYPE>()> new_branches;
        if (hamming_value) {
          std::memcpy(new_branches.data(), m_branches.data(), pos * sizeof(Branch *));
          std::memcpy(new_branches.data() + (pos + 1), m_branches.data() + pos, (hamming_value - pos) * sizeof(Branch *));
          for (size_t i = 0; i != hamming_value; ++i) {
            if (!m_branches[i]->increment_refs()) {
              for (size_t j = 0; j != i; ++j)
                m_branches[j]->decrement_refs();
              new_branch->decrement_refs();
              return nullptr;
            }
          }
        }
        new_branches[pos] = new_branch;
        return ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE, ALLOCATOR>::Create(this->get_bmp() | flag, hamming_value + 1, new_branches);
      }

      const ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE, ALLOCATOR> * updated(const size_t pos, const FLAG_TYPE flag, Branch * const new_branch) const override {
        assert(this->get_bmp() & flag);
        std::array<Branch *, hamming<FLAG_TYPE>()> new_branches;
        if (hamming_value) {
          std::memcpy(new_branches.data(), m_branches.data(), pos * sizeof(Branch *));
          std::memcpy(new_branches.data() + (pos + 1), m_branches.data() + (pos + 1), (hamming_value - pos - 1) * sizeof(Branch *));
          for (size_t i = 0; i != pos; ++i) {
            if (!new_branches[i]->increment_refs()) {
              for (size_t j = 0; j != i; ++j)
                new_branches[j]->decrement_refs();
              new_branch->decrement_refs();
              return nullptr;
            }
          }
          for (size_t i = pos + 1; i != hamming_value; ++i) {
            if (!new_branches[i]->increment_refs()) {
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
        return ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE, ALLOCATOR>::Create(this->get_bmp(), hamming_value, new_branches);
      }

      const ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE, ALLOCATOR> * erased(const size_t pos, const FLAG_TYPE flag) const override {
        assert(this->get_bmp() & flag);
        std::array<Branch *, hamming<FLAG_TYPE>()> new_branches;
        if (hamming_value) {
          std::memcpy(new_branches.data(), m_branches.data(), pos * sizeof(Branch *));
          std::memcpy(new_branches.data() + pos, m_branches.data() + (pos + 1), (hamming_value - pos - 1) * sizeof(Branch *));
          for (size_t i = 0; i != hamming_value - 1; ++i) {
            if (!new_branches[i]->increment_refs()) {
              for (size_t j = 0; j != i; ++j)
                new_branches[j]->decrement_refs();
              return nullptr;
            }
          }
        }
        return ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE, ALLOCATOR>::Create(this->get_bmp() & ~flag, hamming_value - 1, new_branches);
      }

      const ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE, ALLOCATOR> * updated(const Generation<typename ALLOCATOR::Unaligned::Allocator> * const new_generation) const override {
        std::array<Branch *, hamming<FLAG_TYPE>()> new_branches;
        if (hamming_value) {
          std::memcpy(new_branches.data(), m_branches.data(), hamming_value * sizeof(Branch *));
          for (size_t i = 0; i != hamming_value; ++i) {
            if (const Indirection_Node<ALLOCATOR> * const inode = dynamic_cast<Indirection_Node<ALLOCATOR> *>(new_branches[i])) {
              if (!new_generation->increment_refs()) {
                for (size_t j = 0; j != i; ++j)
                  new_branches[j]->decrement_refs();
                return nullptr;
              }
              else
                new_branches[i] = new Indirection_Node<ALLOCATOR>(inode->main.load(std::memory_order_relaxed), new_generation);
            }
            else if (!new_branches[i]->increment_refs()) {
              for (size_t j = 0; j != i; ++j)
                new_branches[j]->decrement_refs();
              return nullptr;
            }
          }
        }
        return ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE, ALLOCATOR>::Create(this->get_bmp(), hamming_value, new_branches);
      }

    private:
      const std::array<Branch *, hamming_value> m_branches;
    };

    template <typename HASH_VALUE_TYPE, typename FLAG_TYPE, typename ALLOCATOR, size_t IN = hamming<FLAG_TYPE>()>
    struct Ctrie_Node_Generator {
      static void Create(std::array<std::function<const ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE, ALLOCATOR> *(const FLAG_TYPE, const std::array<Branch *, hamming<FLAG_TYPE>()> &)>, sum(hamming<FLAG_TYPE>(), FLAG_TYPE(1))> &generator) {
        Ctrie_Node_Generator<HASH_VALUE_TYPE, FLAG_TYPE, ALLOCATOR, IN - 1>::Create(generator);

        generator[IN] = [](const FLAG_TYPE bmp, const std::array<Branch *, hamming<FLAG_TYPE>()> &branches)->ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE, ALLOCATOR> * {
          return new Ctrie_Node<HASH_VALUE_TYPE, FLAG_TYPE, IN, ALLOCATOR>(bmp, branches);
        };
      }
    };

    template <typename HASH_VALUE_TYPE, typename FLAG_TYPE, typename ALLOCATOR>
    struct Ctrie_Node_Generator<HASH_VALUE_TYPE, FLAG_TYPE, ALLOCATOR, 0> {
      static void Create(std::array<std::function<const ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE, ALLOCATOR> *(const FLAG_TYPE, const std::array<Branch *, hamming<FLAG_TYPE>()> &)>, sum(hamming<FLAG_TYPE>(), FLAG_TYPE(1))> &generator) {
        generator[0] = [](const FLAG_TYPE bmp, const std::array<Branch *, hamming<FLAG_TYPE>()> &branches)->ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE, ALLOCATOR> * {
          return new Ctrie_Node<HASH_VALUE_TYPE, FLAG_TYPE, 0, ALLOCATOR>(bmp, branches);
        };
      }
    };

    template <typename HASH_VALUE_TYPE, typename FLAG_TYPE, typename ALLOCATOR>
    ICtrie_Node<HASH_VALUE_TYPE, FLAG_TYPE, ALLOCATOR>::Factory::Factory() {
      Ctrie_Node_Generator<HASH_VALUE_TYPE, FLAG_TYPE, ALLOCATOR>::Create(m_generator);
    }

    template <typename KEY, typename PRED, typename ALLOCATOR>
    struct List_Node : public Main_Node, public Allocated<typename ALLOCATOR::template Aligned<std::align_val_t(ZENI_CONCURRENCY_DESTRUCTIVE_INTERFERENCE_SIZE)>::Allocator> {
    private:
      List_Node(const List_Node &) = delete;
      List_Node & operator=(const List_Node &) = delete;

    public:
      enum class Result {
        Restart,             ///< Ctrie operation must restart due to concurrent activity
        Found,               ///< Lookup successful
        Not_Found,           ///< Lookup failed
        First_Insertion,     ///< Count increases to 1 and object inserted into trie
        Last_Removal,        ///< Count decrements to 0 and object removed from trie
        Replacing_Insertion, ///< Object inserted into trie replaces previous object associated with given key value
        Failed_Removal       ///< Object was not present in the trie and could not be removed
      };

      List_Node(Singleton_Node<KEY, ALLOCATOR> * const snode_, List_Node * const next_ = nullptr) : snode(snode_), next(next_) {}

      ~List_Node() {
        snode->decrement_refs();
        while (next) {
          if (next->decrement_refs() > 1)
            break;
          List_Node * next_next = next->next;
          next->next = nullptr;
          next = next_next;
        }
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
              if (!old_head->next->increment_refs()) {
                while (new_head) {
                  auto next = new_head->next;
                  new_head->next = nullptr;
                  delete new_head;
                  new_head = next;
                  return std::make_tuple(Result::Restart, nullptr, nullptr, nullptr);
                }
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
            if (!old_head->snode->increment_refs()) {
              while (new_head) {
                auto next = new_head->next;
                new_head->next = nullptr;
                delete new_head;
                new_head = next;
                return std::make_tuple(Result::Restart, nullptr, nullptr, nullptr);
              }
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

  template <typename KEY, typename HASH = std::hash<KEY>, typename PRED = std::equal_to<KEY>, typename FLAG_TYPE = uint32_t, typename ALLOCATOR = Jemallocator<char>>
  class Ctrie : public Allocated<typename ALLOCATOR::template Aligned<std::align_val_t(ZENI_CONCURRENCY_DESTRUCTIVE_INTERFERENCE_SIZE)>::Allocator> {
    Ctrie(const Ctrie &) = delete;
    Ctrie operator=(const Ctrie &) = delete;

  public:
    typedef KEY Key;
    typedef HASH Hash;
    typedef PRED Pred;
    typedef ALLOCATOR Allocator;

    typedef decltype(Hash()(Key())) Hash_Value;
    typedef FLAG_TYPE Flag_Type;

    typedef Ctrie_Internal::Generation<Allocator> Gen;
    typedef Ctrie_Internal::Main_or_Failed MoF;
    typedef Ctrie_Internal::Failed_Node<Allocator> FNode;
    typedef Ctrie_Internal::Main_Node MNode;
    typedef Ctrie_Internal::Singleton_Node<Key, Allocator> SNode;
    typedef Ctrie_Internal::ICtrie_Node<Hash_Value, Flag_Type, Allocator> CNode;
    typedef Ctrie_Internal::List_Node<Key, Pred, Allocator> LNode;
    typedef Ctrie_Internal::Branch Branch;
    typedef Ctrie_Internal::Tomb_Node<Allocator> TNode;
    typedef Ctrie_Internal::Indirection_Node<Allocator> INode;
    typedef Ctrie_Internal::RDCSS_Descriptor<Allocator> RDCSSD;

    typedef typename LNode::Result Result;

    Ctrie() = default;

    ~Ctrie() {
      const INode * const inode = m_root.load(std::memory_order_acquire);
      if (inode)
        inode->decrement_refs();
    }

    //bool empty() const {
    //  return !m_root.load(std::memory_order_acquire);
    //}

    //size_t size() const {
    //  size_t sz = 0;
    //  for ([[maybe_unused]] auto &value : *this)
    //    ++sz;
    //  return sz;
    //}

    //bool size_one() const {
    //  auto it = cbegin();
    //  const auto iend = cend();
    //  if (it != iend)
    //    return ++it == iend;
    //  return false;
    //}

    //bool size_zero() const {
    //  return cbegin() == cend();
    //}

    template <typename Comparable, typename CHash = Hash, typename CPred = Pred>
    std::pair<Result, Key> lookup(const Comparable &key) const {
      const Hash_Value hash_value = CHash()(key);
      for (;;) {
        INode * const root = m_root.load(std::memory_order_acquire);
        const auto[result, found] = ilookup<Comparable, CPred>(root, key, hash_value, 0, nullptr, root->generation);
        if (result != Result::Restart)
          return found ? std::make_pair(Result::Found, found->key) : std::make_pair(Result::Not_Found, Key());
      }
    }

    std::tuple<Result, Key, Key> insert(const Key &key) {
      const Hash_Value hash_value = Hash()(key);
      for (;;) {
        INode * const root = m_root.load(std::memory_order_acquire);
        const auto[result, inserted, replaced] = iinsert(root, key, hash_value, 0, nullptr, root->generation);
        if (result != Result::Restart)
          return std::make_tuple(result, inserted ? inserted->key : Key(), replaced ? replaced->key : Key());
      }
    }

    std::pair<Result, Key> erase(const Key &key) {
      const Hash_Value hash_value = Hash()(key);
      for (;;) {
        INode * const root = m_root.load(std::memory_order_acquire);
        const auto[result, removed] = ierase(root, key, hash_value, 0, nullptr, root->generation);
        if (result != Result::Restart)
          return std::make_pair(result, removed ? removed->key : Key());
      }
    }

    void clear() {
      auto other = new INode(CNode::Create(0x0, 0, {}), new Gen);
      other = m_root.exchange(other, std::memory_order_acq_rel);
      other->decrement_refs();
    }

  private:
    template <typename Comparable, typename CPred>
    std::pair<Result, const SNode *> ilookup(
      INode * inode,
      const Key &key,
      const Hash_Value &hash_value,
      size_t level,
      INode * parent,
      const Gen * const root_gen) const
    {
      for (;;) {
        auto inode_main = GCAS_Read(inode);
        if (const auto cnode = dynamic_cast<const CNode *>(inode_main)) {
          const auto[flag, pos] = cnode->flagpos(hash_value, level);
          Branch * const branch = cnode->get_bmp() & flag ? cnode->at(pos) : nullptr;
          if (!branch)
            return std::make_pair(Result::Not_Found, nullptr);
          if (const auto inode_next = dynamic_cast<INode *>(branch)) {
            parent = inode;
            level += CNode::W;
            if (inode_next->generation == root_gen)
              inode = inode_next;
            else {
              const auto updated_cnode = cnode->updated(root_gen);
              if (CAS_del(inode->main, inode_main, updated_cnode, std::memory_order_release, std::memory_order_relaxed))
                inode = dynamic_cast<INode *>(updated_cnode->at(pos));
              else
                return std::make_pair(Result::Restart, nullptr);
            }
            continue;
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
      INode * parent,
      const Gen * const root_gen)
    {
      for (;;) {
        auto inode_main = GCAS_Read(inode);
        if (const auto cnode = dynamic_cast<const CNode *>(inode_main)) {
          const auto[flag, pos] = cnode->flagpos(hash_value, level);
          Branch * branch = cnode->get_bmp() & flag ? cnode->at(pos) : nullptr;
          if (!branch) {
            const auto new_snode = new SNode(key);
            const auto new_cnode = cnode->inserted(pos, flag, new_snode);
            if (!new_cnode)
              return std::make_tuple(Result::Restart, nullptr, nullptr);
            if (GCAS(inode, inode_main, new_cnode))
              return std::make_tuple(Result::First_Insertion, new_snode, nullptr);
            else
              return std::make_tuple(Result::Restart, nullptr, nullptr);
          }
          else {
            if (const auto inode_next = dynamic_cast<INode *>(branch)) {
              parent = inode;
              level += CNode::W;
              if (inode_next->generation == root_gen)
                inode = inode_next;
              else {
                const auto updated_cnode = cnode->updated(root_gen);
                if (CAS_del(inode->main, inode_main, updated_cnode, std::memory_order_release, std::memory_order_relaxed))
                  inode = dynamic_cast<INode *>(updated_cnode->at(pos));
                else
                  return std::make_tuple(Result::Restart, nullptr, nullptr);
              }
              continue;
            }
            else if (const auto snode = dynamic_cast<SNode *>(branch)) {
              const auto new_snode = new SNode(key);
              if (Pred()(snode->key, key)) {
                const auto new_cnode = cnode->updated(pos, flag, new_snode);
                if (!new_cnode)
                  return std::make_tuple(Result::Restart, nullptr, nullptr);
                if (GCAS(inode, inode_main, new_cnode))
                  return std::make_tuple(Result::Replacing_Insertion, new_snode, snode);
                else
                  return std::make_tuple(Result::Restart, nullptr, nullptr);
              }
              else {
                const Hash_Value snode_hash = Hash()(snode->key);
                if (snode_hash != hash_value) {
                  if (!snode->increment_refs())
                    return std::make_tuple(Result::Restart, nullptr, nullptr);
                  if (!inode->generation->increment_refs()) {
                    snode->decrement_refs();
                    return std::make_tuple(Result::Restart, nullptr, nullptr);
                  }
                  const auto new_cnode = cnode->updated(pos, flag, new INode(CNode::Create(snode, snode_hash, new_snode, hash_value, level + CNode::W, inode->generation), inode->generation));
                  if (!new_cnode)
                    return std::make_tuple(Result::Restart, nullptr, nullptr);
                  if (GCAS(inode, inode_main, new_cnode))
                    return std::make_tuple(Result::First_Insertion, new_snode, nullptr);
                  else
                    return std::make_tuple(Result::Restart, nullptr, nullptr);
                }
                else {
                  if (!snode->increment_refs())
                    return std::make_tuple(Result::Restart, nullptr, nullptr);
                  if (!inode->generation->increment_refs()) {
                    snode->decrement_refs();
                    return std::make_tuple(Result::Restart, nullptr, nullptr);
                  }
                  const auto new_cnode = cnode->updated(pos, flag, new INode(new LNode(new_snode, new LNode(snode)), inode->generation));
                  if (!new_cnode)
                    return std::make_tuple(Result::Restart, nullptr, nullptr);
                  if (GCAS(inode, inode_main, new_cnode))
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
            if (!lnode->increment_refs())
              return std::make_tuple(Result::Restart, nullptr, nullptr);
            if (!inode->generation->increment_refs()) {
              lnode->decrement_refs();
              return std::make_tuple(Result::Restart, nullptr, nullptr);
            }
            const auto new_snode = new SNode(key);
            const auto new_cnode = CNode::Create(new_snode, hash_value, new INode(lnode, inode->generation), lnode_hash, level, inode->generation);
            if (!new_cnode)
              return std::make_tuple(Result::Restart, nullptr, nullptr);
            if (GCAS(inode, inode_main, new_cnode))
              return std::make_tuple(Result::First_Insertion, new_snode, nullptr);
            else
              return std::make_tuple(Result::Restart, nullptr, nullptr);
          }
          else {
            const auto[result, new_lnode, inserted, replaced] = lnode->updated(key, true);
            if (GCAS(inode, inode_main, new_lnode))
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
      INode * parent,
      const Gen * const root_gen)
    {
      for (;;) {
        auto inode_main = GCAS_Read(inode);
        if (const auto cnode = dynamic_cast<const CNode *>(inode_main)) {
          const auto[flag, pos] = cnode->flagpos(hash_value, level);
          Branch * branch = cnode->get_bmp() & flag ? cnode->at(pos) : nullptr;
          if (!branch)
            return std::make_pair(Result::Not_Found, nullptr);
          if (const auto inode_next = dynamic_cast<INode *>(branch)) {
            parent = inode;
            level += CNode::W;
            if (inode_next->generation == root_gen)
              inode = inode_next;
            else {
              const auto updated_cnode = cnode->updated(root_gen);
              if (CAS_del(inode->main, inode_main, updated_cnode, std::memory_order_release, std::memory_order_relaxed))
                inode = dynamic_cast<INode *>(updated_cnode->at(pos));
              else
                return std::make_pair(Result::Restart, nullptr);
            }
            continue;
          }
          else if (const auto snode = dynamic_cast<const SNode *>(branch)) {
            if (Pred()(key, snode->key)) {
              const CNode * new_cnode = cnode->erased(pos, flag);
              if (!new_cnode)
                return std::make_pair(Result::Restart, nullptr);
              const MNode * new_mainnode = new_cnode->to_contracted(level);
              if (GCAS(inode, inode_main, new_mainnode)) {
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
          if (result == Result::Failed_Removal) {
            delete new_lnode;
            return std::make_pair(Result::Failed_Removal, nullptr);
          }
          MNode * new_mainnode = new_lnode;
          if (!new_lnode->next) {
            [[maybe_unused]] const int64_t refs = new_lnode->snode->increment_refs();
            assert(refs);
            new_mainnode = new TNode(new_lnode->snode);
            delete new_lnode;
          }
          if (GCAS(inode, inode_main, new_mainnode))
            return std::make_pair(Result::Last_Removal, removed);
          else
            return std::make_pair(Result::Restart, nullptr);
        }
        else
          abort();
      }
    }

    void clean(INode * const inode, const size_t level) const {
      const MNode * inode_main = GCAS_Read(inode);
      if (auto cnode = dynamic_cast<const CNode *>(inode_main)) {
        if (const MNode * const compressed = cnode->to_compressed(level))
          GCAS(inode, inode_main, compressed);
      }
    }

    void clean_parent(INode * const parent, INode * const inode, const Hash_Value &hash_value, const size_t level) {
      do {
        const MNode * parent_main = GCAS_Read(parent);
        if (const CNode * const cnode = dynamic_cast<const CNode *>(parent_main)) {
          const auto[flag, pos] = cnode->flagpos(hash_value, level);
          const Branch * const branch = cnode->get_bmp() & flag ? cnode->at(pos) : nullptr;
          if (branch == inode) {
            const MNode * const inode_main = GCAS_Read(inode);
            if (const TNode * const tnode = dynamic_cast<const TNode *>(inode_main)) {
              Branch * const resurrected = inode->resurrect();
              if (!resurrected)
                continue;
              const auto new_cnode = cnode->updated(pos, flag, resurrected)->to_contracted(level);
              if (!new_cnode) {
                resurrected->decrement_refs();
                continue;
              }
              if (!GCAS(parent, parent_main, new_cnode))
                continue;
            }
          }
        }
      } while (false);
    }

    bool GCAS(INode * const inode, const MNode * old_mnode, const MNode * const new_mnode) const {
      if (!old_mnode->increment_refs()) {
        delete new_mnode;
        return false;
      }
      new_mnode->prev.store(old_mnode, std::memory_order_relaxed);
      if (CAS_del(inode->main, old_mnode, new_mnode, std::memory_order_release, std::memory_order_relaxed)) {
        GCAS_Commit(inode, new_mnode);
        return !new_mnode->prev.load(std::memory_order_relaxed);
      }
      else
        return false;
    }

    const MNode * GCAS_Commit(INode * const inode, const MNode * mnode) const {
      for (;;) {
        const MoF * prev_mof = mnode->prev.load(std::memory_order_relaxed);
        INode * const root = Abortable_Read_Root();
        if (const MNode * const prev_mnode = dynamic_cast<const MNode *>(prev_mof)) {
          if (root->generation == inode->generation && !m_readonly) {
            if (CAS(mnode->prev, prev_mof, static_cast<const MNode *>(nullptr), std::memory_order_release, std::memory_order_acquire))
              return mnode;
          }
          else {
            if (prev_mnode->increment_refs())
              CAS_del(mnode->prev, prev_mof, new FNode(prev_mnode), std::memory_order_release, std::memory_order_acquire);
            mnode = inode->main.load(std::memory_order_acquire);
          }
        }
        else if (const FNode * const prev_failed_node = dynamic_cast<const FNode *>(prev_mof)) {
          if (!prev_failed_node->prev->increment_refs())
            mnode = inode->main.load(std::memory_order_acquire);
          else if (CAS(inode->main, mnode, prev_failed_node->prev, std::memory_order_release, std::memory_order_acquire))
            return prev_failed_node->prev;
          else
            mnode = inode->main.load(std::memory_order_acquire);
        }
        else {
          assert(!prev_mof);
          return mnode;
        }
      }
    }

    const MNode * GCAS_Read(INode * const inode) const {
      const MNode * const mnode = inode->main.load(std::memory_order_acquire);
      const MoF * const prev_mof = mnode->prev.load(std::memory_order_relaxed);
      if (!prev_mof)
        return mnode;
      else
        return GCAS_Commit(inode, mnode);
    }

    INode * const Abortable_Read_Root() const {
      return m_root.load(std::memory_order_acquire);
    }

    bool RDCSS(std::atomic<MNode *> &branch, RDCSSD * const rdcss_descriptor) {
      for (;;) {
        MNode * expected = rdcss_descriptor->expected_mnode;
        if (CAS(branch, expected, rdcss_descriptor, std::memory_order_release, std::memory_order_acquire)) {
          expected = rdcss_descriptor;
          RDCSS_Complete(branch, expected, rdcss_descriptor);
          return true;
        }
        else if (const RDCSSD * const other_descriptor = dynamic_cast<RDCSSD *>(expected)) {
          RDCSS_Complete(branch, expected, other_descriptor);
          continue;
        }
        else
          return false;
      }
    }

    void RDCSS_Commit(std::atomic<MNode *> &branch, MNode * &expected_rdcss_descriptor, const RDCSSD * const rdcss_descriptor) const {
      assert(expected_rdcss_descriptor == rdcss_descriptor);
      if (m_root.main.load(std::memory_order_relaxed) == rdcss_descriptor->expected_root)
        CAS(branch, expected_rdcss_descriptor, rdcss_descriptor->desired_mnode, std::memory_order_relaxed, std::memory_order_relaxed);
      else
        CAS(branch, expected_rdcss_descriptor, rdcss_descriptor->expected_mnode, std::memory_order_relaxed, std::memory_order_relaxed);
    }

    bool RDCSS_Read(std::atomic<MNode *> &branch) const {
      for (;;) {
        MNode * expected = branch.load(std::memory_order_acquire);
        if (const RDCSSD * const rdcss_descriptor = dynamic_cast<RDCSSD *>(expected)) {
          RDCSS_Complete(branch, expected, rdcss_descriptor);
          continue;
        }
        else
          return expected;
      }
    }

    template <typename VALUE_TYPE, typename DESIRED>
    static bool CAS(std::atomic<VALUE_TYPE *> &atomic_value, VALUE_TYPE * &expected, const DESIRED * desired, const std::memory_order success = std::memory_order_seq_cst, const std::memory_order failure = std::memory_order_seq_cst) {
      if (atomic_value.compare_exchange_strong(expected, desired, success, failure)) {
        expected->decrement_refs();
        return true;
      }
      else {
        if (desired)
          desired->decrement_refs();
        return false;
      }
    }

    template <typename VALUE_TYPE, typename DESIRED>
    static bool CAS_del(std::atomic<VALUE_TYPE *> &atomic_value, VALUE_TYPE * &expected, const DESIRED * desired, const std::memory_order success = std::memory_order_seq_cst, const std::memory_order failure = std::memory_order_seq_cst) {
      if (atomic_value.compare_exchange_strong(expected, desired, success, failure)) {
        expected->decrement_refs();
        return true;
      }
      else {
        delete desired;
        return false;
      }
    }

    ZENI_CONCURRENCY_CACHE_ALIGN std::atomic<INode *> m_root = new INode(CNode::Create(0x0, 0, {}), new Gen);
    bool m_readonly = false;
  };

}

#endif
