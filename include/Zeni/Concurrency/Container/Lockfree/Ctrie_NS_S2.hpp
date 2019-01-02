#ifndef ZENI_CONCURRENCY_CTRIE_NS_S2_HPP
#define ZENI_CONCURRENCY_CTRIE_NS_S2_HPP

#include "Intrusive_Shared_Ptr.hpp"

#include <array>
#include <cstring>
#include <functional>
#include <stack>

namespace Zeni::Concurrency {

  template <typename KEY, typename SUBTRIE, typename HASH = std::hash<KEY>, typename PRED = std::equal_to<KEY>, typename FLAG_TYPE = uint32_t>
  class Ctrie_NS_S2;

  namespace Ctrie_NS_S2_Internal {

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

    struct Early_Decrement_Propagation : public Enable_Intrusive_Sharing {
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

    struct Node : public Early_Decrement_Propagation {
    private:
      Node(const Node &) = delete;
      Node & operator=(const Node &) = delete;

    public:
      Node() = default;
    };

    struct Main_Node : public Node {
    private:
      Main_Node(const Main_Node &) = delete;
      Main_Node & operator=(const Main_Node &) = delete;

    public:
      Main_Node() = default;
    };

    struct Branch : public Node {
    private:
      Branch(const Branch &) = delete;
      Branch & operator=(const Branch &) = delete;

    public:
      Branch() = default;

      virtual Branch * resurrect() {
        abort();
      }
    };

    struct Tomb_Node : public Main_Node {
    private:
      Tomb_Node(const Tomb_Node &) = delete;
      Tomb_Node & operator=(const Tomb_Node &) = delete;

    public:
      Tomb_Node() = default;

      Tomb_Node(Branch * const branch_) : branch(branch_) {}

      void on_final_decrement() override {
        Main_Node::on_final_decrement();
        if (branch)
          branch->decrement_refs();
      }

      Branch * const branch;
    };

    template <typename KEY, typename SUBTRIE>
    struct Singleton_Node : public Branch {
    private:
      Singleton_Node(const Singleton_Node &) = delete;
      Singleton_Node & operator=(const Singleton_Node &) = delete;

      template <typename KEY_TYPE>
      Singleton_Node(KEY_TYPE &&key_) : key(std::forward<KEY_TYPE>(key_)) {}

    public:
      template <size_t index, typename KEY_TYPE, typename VALUE_TYPE>
      static auto Create_Insert(KEY_TYPE &&key_, VALUE_TYPE &&value_) {
        auto snode = new Singleton_Node(key_);
        auto tuple_value = snode->subtrie.template insert<index>(std::forward<VALUE_TYPE>(value_));
        return std::make_pair(tuple_value, snode);
      }

      template <size_t index, typename KEY_TYPE, typename VALUE_TYPE>
      static auto Create_Erase(KEY_TYPE &&key_, VALUE_TYPE &&value_) {
        auto snode = new Singleton_Node(key_);
        auto tuple_value = snode->subtrie.template erase<index>(std::forward<VALUE_TYPE>(value_));
        if (snode->subtrie.empty()) {
          delete snode;
          return std::make_pair(tuple_value, static_cast<Singleton_Node *>(nullptr));
        }
        else
          return std::make_pair(tuple_value, snode);
      }

      template <size_t index>
      auto insert(const KEY &key, const typename std::tuple_element_t<index, typename SUBTRIE::Types>::Key &value) const {
        return subtrie.template insert<index>(value);
      }

      template <size_t index>
      auto insert_ip(const KEY &key, const typename std::tuple_element_t<index, typename SUBTRIE::Types>::Key &value) const {
        return subtrie.template insert_ip<index>(value);
      }

      template <size_t index>
      auto erase(const KEY &key, const typename std::tuple_element_t<index, typename SUBTRIE::Types>::Key &value) const {
        return subtrie.template erase<index>(value);
      }

      template <size_t index>
      auto erase_ip(const KEY &key, const typename std::tuple_element_t<index, typename SUBTRIE::Types>::Key &value) const {
        return subtrie.template erase_ip<index>(value);
      }

      template <size_t src, size_t dest>
      auto move(const KEY &key, const typename std::tuple_element_t<src, typename SUBTRIE::Types>::Key &value) const {
        return subtrie.template move<src, dest>(value);
      }

      template <size_t if_present, size_t regardless>
      auto insert_ip_xp(const KEY &key, const typename std::tuple_element_t<if_present, typename SUBTRIE::Types>::Key &value) const {
        return subtrie.template insert_ip_xp<if_present, regardless>(value);
      }

      template <size_t if_present, size_t regardless>
      auto erase_ip_xp(const KEY &key, const typename std::tuple_element_t<if_present, typename SUBTRIE::Types>::Key &value) const {
        return subtrie.template erase_ip_xp<if_present, regardless>(value);
      }

      Branch * resurrect() override {
        return subtrie.valid() ? (this->try_increment_refs() ? this : nullptr) : reinterpret_cast<Branch *>(0x1);
      }

      const KEY key;
      mutable SUBTRIE subtrie = true;
    };

    template <typename KEY, typename SUBTRIE>
    struct Indirection_Node : public Branch {
    private:
      Indirection_Node(const Indirection_Node &) = delete;
      Indirection_Node & operator=(const Indirection_Node &) = delete;

    public:
      Indirection_Node(Main_Node * const main_)
        : main(main_)
      {
      }

      void on_final_decrement() override {
        Main_Node * const m = ptr_part(main.load(std::memory_order_acquire));
        if (m)
          m->decrement_refs();
      }

      static bool is_marked_readonly(Main_Node * const ptr) {
        return reinterpret_cast<uintptr_t>(ptr) & 0x1;
      }
      static Main_Node * ptr_part(Main_Node * const ptr) {
        return reinterpret_cast<Main_Node *>(reinterpret_cast<uintptr_t>(ptr) & ~0x3);
      }
      static Main_Node * mark_readonly(Main_Node * const ptr) {
        const uintptr_t ival = reinterpret_cast<uintptr_t>(ptr) & ~0x3;
        return ival & 0x3 ? ptr : reinterpret_cast<Main_Node *>(reinterpret_cast<uintptr_t>(ptr) | 0x1);
      }

      Branch * resurrect() override {
        const auto mnode = main.load(std::memory_order_acquire);
        if (const auto tnode = dynamic_cast<const Tomb_Node *>(mnode))
          return tnode->branch ? (tnode->branch->try_increment_refs() ? tnode->branch : nullptr) : reinterpret_cast<Branch *>(0x1);
        if (const auto snode = dynamic_cast<const Singleton_Node<KEY, SUBTRIE> *>(mnode)) {
          if (!snode->subtrie.valid())
            return reinterpret_cast<Branch *>(0x1);
        }
        return this->try_increment_refs() ? this : nullptr;
      }

      ZENI_CONCURRENCY_CACHE_ALIGN std::atomic<Main_Node *> main;
    };

    template <typename KEY, typename SUBTRIE, typename HASH, typename PRED, typename FLAG_TYPE>
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
      typedef decltype(HASH()(KEY())) Hash_Value;
      static const FLAG_TYPE W = log2(hamming_max);

      static ICtrie_Node * Create(const FLAG_TYPE bmp, const size_t hamming_value, const std::array<Branch *, hamming_max> &branches) {
        static Factory factory;
        return factory.create(bmp, hamming_value, branches);
      }

      static ICtrie_Node * Create(Branch * const first, const Hash_Value first_hash, Branch * const second, const Hash_Value second_hash, const size_t level) {
        assert(first_hash != second_hash);
        const auto first_flag = flag(first_hash, level);
        const auto second_flag = flag(second_hash, level);
        if (first_flag == second_flag) {
          std::array<Branch *, hamming_max> branches;
          branches[0] = new Indirection_Node<KEY, SUBTRIE>(Create(first, first_hash, second, second_hash, level + W));
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

      std::pair<FLAG_TYPE, size_t> flagpos(const Hash_Value hash_value, const size_t level) const {
        const FLAG_TYPE desired_bit = flag(hash_value, level);
        const size_t array_index = hamming(m_bmp & (desired_bit - 1));
        return std::make_pair(desired_bit, array_index);
      }

      virtual Branch * at(const size_t i) const = 0;

      virtual ICtrie_Node * inserted(const size_t pos, const FLAG_TYPE flag, Branch * const new_branch) const = 0;

      virtual ICtrie_Node * updated(const size_t pos, const FLAG_TYPE flag, Branch * const new_branch) const = 0;

      virtual ICtrie_Node * erased(const size_t pos, const FLAG_TYPE flag) const = 0;

      Main_Node * to_compressed(const size_t level) {
        FLAG_TYPE new_bmp = get_bmp();
        size_t new_hamming_value = 0;
        std::array<size_t, hamming_max> desired_bits;
        for (size_t desired_bit_index = 0; desired_bit_index != hamming_max; ++desired_bit_index) {
          const FLAG_TYPE desired_bit = FLAG_TYPE(1u) << desired_bit_index;
          if (new_bmp & desired_bit)
            desired_bits[new_hamming_value++] = desired_bit_index;
        }
        assert(new_hamming_value = get_hamming_value());

        bool is_compressed = false;
        std::array<Branch *, hamming_max> branches;
        for (size_t i = 0, j = 0; i != get_hamming_value(); ++i) {
          Branch * const resurrected = at(i)->resurrect();
          if (!resurrected) {
            for (size_t k = 0; k != j; ++k)
              branches[k]->decrement_refs();
            return nullptr;
          }
          if (uintptr_t(resurrected) == 0x1) {
            new_bmp ^= FLAG_TYPE(1u) << desired_bits[i];
            --new_hamming_value;
            is_compressed = true;
          }
          else {
            branches[j++] = resurrected;
            is_compressed |= resurrected != at(i);
          }
        }
        if (is_compressed)
          return Create(new_bmp, new_hamming_value, branches)->to_contracted(level);
        else {
          for (size_t i = 0; i != new_hamming_value; ++i)
            branches[i]->decrement_refs();
          return nullptr;
        }
      }

      Main_Node * to_contracted(const size_t level) {
        if (level && get_hamming_value() == 1) {
          Branch * const branch = at(0);
          [[maybe_unused]] const int64_t refs = branch->try_increment_refs();
          assert(refs);
          this->immediate_deletion();
          return new Tomb_Node(branch);
        }
        else if (level && get_hamming_value() == 0) {
          this->immediate_deletion();
          return new Tomb_Node(nullptr);
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

        ICtrie_Node<KEY, SUBTRIE, HASH, PRED, FLAG_TYPE> * create(const FLAG_TYPE bmp, const size_t hamming_value, const std::array<Branch *, hamming_max> &branches) {
          return m_generator[hamming_value](bmp, branches);
        }

      private:
        std::array<std::function<ICtrie_Node<KEY, SUBTRIE, HASH, PRED, FLAG_TYPE> *(const FLAG_TYPE, const std::array<Branch *, hamming_max> &)>, sum(hamming_max, FLAG_TYPE(1))> m_generator;
      };

      static FLAG_TYPE flag(const Hash_Value hash_value, const size_t level) {
        const Hash_Value shifted_hash = hash_value >> level;
        const Hash_Value desired_bit_index = shifted_hash & unhamming_filter;
        const FLAG_TYPE desired_bit = FLAG_TYPE(1u) << desired_bit_index;
        return desired_bit;
      }
    };

    template <typename KEY, typename SUBTRIE, typename HASH, typename PRED, typename FLAG_TYPE, size_t HAMMING_VALUE>
    struct Ctrie_Node : public ICtrie_Node<KEY, SUBTRIE, HASH, PRED, FLAG_TYPE> {
    private:
      Ctrie_Node(const Ctrie_Node &) = delete;
      Ctrie_Node & operator=(const Ctrie_Node &) = delete;

    public:
      typedef decltype(HASH()(KEY())) Hash_Value;

      static const size_t hamming_value = HAMMING_VALUE;

      Ctrie_Node(const FLAG_TYPE bmp, const std::array<Branch *, hamming<FLAG_TYPE>()> &branches)
        : ICtrie_Node<KEY, SUBTRIE, HASH, PRED, FLAG_TYPE>(bmp),
        m_branches(reinterpret_cast<const std::array<Branch *, hamming_value> &>(branches)) //< Should always be smaller, safe to copy subset
      {
      }

      ~Ctrie_Node() {
        for (auto branch : m_branches)
          branch->decrement_refs();
      }

      void on_final_decrement() override {
        Main_Node::on_final_decrement();
        //for (auto branch : m_branches)
        //  branch->decrement_refs();
      }

      size_t get_hamming_value() const override {
        return hamming_value;
      };

      Branch * at(const size_t i) const override {
        assert(i >= 0 && i < m_branches.size());
        return m_branches[i];
      }

      ICtrie_Node<KEY, SUBTRIE, HASH, PRED, FLAG_TYPE> * inserted(const size_t pos, const FLAG_TYPE flag, Branch * const new_branch) const override {
        assert(!(this->get_bmp() & flag));
        std::array<Branch *, hamming<FLAG_TYPE>()> new_branches;
        if (hamming_value) {
          std::memcpy(new_branches.data(), m_branches.data(), pos * sizeof(Branch *));
          std::memcpy(new_branches.data() + (pos + 1), m_branches.data() + pos, (hamming_value - pos) * sizeof(Branch *));
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
        return ICtrie_Node<KEY, SUBTRIE, HASH, PRED, FLAG_TYPE>::Create(this->get_bmp() | flag, hamming_value + 1, new_branches);
      }

      ICtrie_Node<KEY, SUBTRIE, HASH, PRED, FLAG_TYPE> * updated(const size_t pos, const FLAG_TYPE flag, Branch * const new_branch) const override {
        assert(this->get_bmp() & flag);
        std::array<Branch *, hamming<FLAG_TYPE>()> new_branches;
        if (hamming_value) {
          std::memcpy(new_branches.data(), m_branches.data(), pos * sizeof(Branch *));
          std::memcpy(new_branches.data() + (pos + 1), m_branches.data() + (pos + 1), (hamming_value - pos - 1) * sizeof(Branch *));
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
        return ICtrie_Node<KEY, SUBTRIE, HASH, PRED, FLAG_TYPE>::Create(this->get_bmp(), hamming_value, new_branches);
      }

      ICtrie_Node<KEY, SUBTRIE, HASH, PRED, FLAG_TYPE> * erased(const size_t pos, const FLAG_TYPE flag) const override {
        assert(this->get_bmp() & flag);
        std::array<Branch *, hamming<FLAG_TYPE>()> new_branches;
        if (hamming_value) {
          std::memcpy(new_branches.data(), m_branches.data(), pos * sizeof(Branch *));
          std::memcpy(new_branches.data() + pos, m_branches.data() + (pos + 1), (hamming_value - pos - 1) * sizeof(Branch *));
          for (size_t i = 0; i != hamming_value - 1; ++i) {
            if (!new_branches[i]->try_increment_refs()) {
              for (size_t j = 0; j != i; ++j)
                new_branches[j]->decrement_refs();
              return nullptr;
            }
          }
        }
        return ICtrie_Node<KEY, SUBTRIE, HASH, PRED, FLAG_TYPE>::Create(this->get_bmp() & ~flag, hamming_value - 1, new_branches);
      }

    private:
      const std::array<Branch *, hamming_value> m_branches;
    };

    template <typename KEY, typename SUBTRIE, typename HASH, typename PRED, typename FLAG_TYPE, size_t IN = hamming<FLAG_TYPE>()>
    struct Ctrie_Node_Generator {
      static void Create(std::array<std::function<ICtrie_Node<KEY, SUBTRIE, HASH, PRED, FLAG_TYPE> *(const FLAG_TYPE, const std::array<Branch *, hamming<FLAG_TYPE>()> &)>, sum(hamming<FLAG_TYPE>(), FLAG_TYPE(1))> &generator) {
        Ctrie_Node_Generator<KEY, SUBTRIE, HASH, PRED, FLAG_TYPE, IN - 1>::Create(generator);

        generator[IN] = [](const FLAG_TYPE bmp, const std::array<Branch *, hamming<FLAG_TYPE>()> &branches)->ICtrie_Node<KEY, SUBTRIE, HASH, PRED, FLAG_TYPE> * {
          return new Ctrie_Node<KEY, SUBTRIE, HASH, PRED, FLAG_TYPE, IN>(bmp, branches);
        };
      }
    };

    template <typename KEY, typename SUBTRIE, typename HASH, typename PRED, typename FLAG_TYPE>
    struct Ctrie_Node_Generator<KEY, SUBTRIE, HASH, PRED, FLAG_TYPE, 0> {
      static void Create(std::array<std::function<ICtrie_Node<KEY, SUBTRIE, HASH, PRED, FLAG_TYPE> *(const FLAG_TYPE, const std::array<Branch *, hamming<FLAG_TYPE>()> &)>, sum(hamming<FLAG_TYPE>(), FLAG_TYPE(1))> &generator) {
        generator[0] = [](const FLAG_TYPE bmp, const std::array<Branch *, hamming<FLAG_TYPE>()> &branches)->ICtrie_Node<KEY, SUBTRIE, HASH, PRED, FLAG_TYPE> * {
          return new Ctrie_Node<KEY, SUBTRIE, HASH, PRED, FLAG_TYPE, 0>(bmp, branches);
        };
      }
    };

    template <typename KEY, typename SUBTRIE, typename HASH, typename PRED, typename FLAG_TYPE>
    ICtrie_Node<KEY, SUBTRIE, HASH, PRED, FLAG_TYPE>::Factory::Factory() {
      Ctrie_Node_Generator<KEY, SUBTRIE, HASH, PRED, FLAG_TYPE>::Create(m_generator);
    }

    template <typename KEY, typename SUBTRIE, typename PRED>
    struct List_Node : public Main_Node {
    private:
      List_Node(const List_Node &) = delete;
      List_Node & operator=(const List_Node &) = delete;

    public:
      List_Node(Singleton_Node<KEY, SUBTRIE> * const snode_, List_Node * const next_ = nullptr)
        : snode(snode_), next(next_)
      {
        assert(snode);
      }

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
        Main_Node::on_final_decrement();
        snode->decrement_refs();
      }

      template <size_t index>
      auto insert(const KEY &key, const typename std::tuple_element_t<index, typename SUBTRIE::Types>::Key &value) {
        if (const auto found = find(key)) {
          const auto tuple_value = found->template insert<index>(key, value);
          if (std::get<1>(tuple_value).valid())
            return std::make_pair(tuple_value, reinterpret_cast<List_Node *>(0x1));
          else {
            const auto new_lnode = erased(found);
            if (uintptr_t(new_lnode) == 0x1)
              return std::make_pair(tuple_value, reinterpret_cast<List_Node *>(0x1));
            else
              return std::make_pair(tuple_value, reinterpret_cast<List_Node *>(uintptr_t(new_lnode) | 0x2));
          }
        }
        else {
          if (!try_increment_refs())
            return std::make_pair(std::make_tuple(std::tuple_element_t<index, typename SUBTRIE::Types>::Result::Restart, typename SUBTRIE::Snapshot(), KEY()), static_cast<List_Node *>(nullptr));
          const auto[tuple_value, new_snode] = Singleton_Node<KEY, SUBTRIE>::template Create_Insert<index>(key, value);
          if (new_snode)
            return std::make_pair(tuple_value, new List_Node(new_snode, this));
          else {
            decrement_refs();
            return std::make_pair(tuple_value, reinterpret_cast<List_Node *>(0x1));
          }
        }
      }

      template <size_t index>
      auto insert_ip(const KEY &key, const typename std::tuple_element_t<index, typename SUBTRIE::Types>::Key &value) {
        if (const auto found = find(key))
          return std::make_pair(found->template insert_ip<index>(key, value), reinterpret_cast<List_Node *>(0x1));
        else
          return std::make_pair(std::make_tuple(std::tuple_element_t<index, typename SUBTRIE::Types>::Result::Not_Present, SUBTRIE::Create_Invalid(), KEY()), reinterpret_cast<List_Node *>(0x1));
      }

      template <size_t if_present, size_t regardless>
      auto insert_ip_xp(const KEY &key, const typename std::tuple_element_t<if_present, typename SUBTRIE::Types>::Key &value) {
        if (const auto found = find(key)) {
          const auto tuple_value = found->template insert_ip_xp<if_present, regardless>(key, value);
          if (std::get<1>(tuple_value).valid())
            return std::make_pair(tuple_value, reinterpret_cast<List_Node *>(0x1));
          else {
            const auto new_lnode = erased(found);
            if (uintptr_t(new_lnode) == 0x1)
              return std::make_pair(tuple_value, reinterpret_cast<List_Node *>(0x1));
            else
              return std::make_pair(tuple_value, reinterpret_cast<List_Node *>(uintptr_t(new_lnode) | 0x2));
          }
        }
        else {
          if (!try_increment_refs())
            return std::make_pair(std::make_tuple(std::tuple_element_t<if_present, typename SUBTRIE::Types>::Result::Restart, typename SUBTRIE::Snapshot(), KEY()), static_cast<List_Node *>(nullptr));
          const auto[tuple_value, new_snode] = Singleton_Node<KEY, SUBTRIE>::template Create_Insert<regardless>(key, value);
          if (new_snode)
            return std::make_pair(tuple_value, new List_Node(new_snode, this));
          else {
            decrement_refs();
            return std::make_pair(tuple_value, reinterpret_cast<List_Node *>(0x1));
          }
        }
      }

      template <size_t index>
      auto erase(const KEY &key, const typename std::tuple_element_t<index, typename SUBTRIE::Types>::Key &value) {
        if (const auto found = find(key)) {
          const auto tuple_value = found->template erase<index>(key, value);
          if (std::get<1>(tuple_value).valid())
            return std::make_pair(tuple_value, reinterpret_cast<List_Node *>(0x1));
          else {
            const auto new_lnode = erased(found);
            if (uintptr_t(new_lnode) == 0x1)
              return std::make_pair(tuple_value, reinterpret_cast<List_Node *>(0x1));
            else
              return std::make_pair(tuple_value, reinterpret_cast<List_Node *>(uintptr_t(new_lnode) | 0x2));
          }
        }
        else {
          if (!try_increment_refs())
            return std::make_pair(std::make_tuple(std::tuple_element_t<index, typename SUBTRIE::Types>::Result::Restart, typename SUBTRIE::Snapshot(), KEY()), static_cast<List_Node *>(nullptr));
          const auto[tuple_value, new_snode] = Singleton_Node<KEY, SUBTRIE>::template Create_Erase<index>(key, value);
          if (new_snode)
            return std::make_pair(tuple_value, new List_Node(new_snode, this));
          else {
            decrement_refs();
            return std::make_pair(tuple_value, reinterpret_cast<List_Node *>(0x1));
          }
        }
      }

      template <size_t index>
      auto erase_ip(const KEY &key, const typename std::tuple_element_t<index, typename SUBTRIE::Types>::Key &value) {
        if (const auto found = find(key))
          return std::make_pair(found->template erase_ip<index>(key, value), reinterpret_cast<List_Node *>(0x1));
        else
          return std::make_pair(std::make_tuple(std::tuple_element_t<index, typename SUBTRIE::Types>::Result::Not_Present, SUBTRIE::Create_Invalid(), KEY()), reinterpret_cast<List_Node *>(0x1));
      }

      template <size_t if_present, size_t regardless>
      auto erase_ip_xp(const KEY &key, const typename std::tuple_element_t<if_present, typename SUBTRIE::Types>::Key &value) {
        if (const auto found = find(key)) {
          const auto tuple_value = found->template erase_ip_xp<if_present, regardless>(key, value);
          if (std::get<1>(tuple_value).valid())
            return std::make_pair(tuple_value, reinterpret_cast<List_Node *>(0x1));
          else {
            const auto new_lnode = erased(found);
            if (uintptr_t(new_lnode) == 0x1)
              return std::make_pair(tuple_value, reinterpret_cast<List_Node *>(0x1));
            else
              return std::make_pair(tuple_value, reinterpret_cast<List_Node *>(uintptr_t(new_lnode) | 0x2));
          }
        }
        else {
          if (!try_increment_refs())
            return std::make_pair(std::make_tuple(std::tuple_element_t<if_present, typename SUBTRIE::Types>::Result::Restart, typename SUBTRIE::Snapshot(), KEY()), static_cast<List_Node *>(nullptr));
          const auto[tuple_value, new_snode] = Singleton_Node<KEY, SUBTRIE>::template Create_Erase<regardless>(key, value);
          if (new_snode)
            return std::make_pair(tuple_value, new List_Node(new_snode, this));
          else {
            decrement_refs();
            return std::make_pair(tuple_value, reinterpret_cast<List_Node *>(0x1));
          }
        }
      }

      template <size_t src, size_t dest>
      auto move(const KEY &key, const typename std::tuple_element_t<src, typename SUBTRIE::Types>::Key &value) {
        if (const auto found = find(key))
          return std::make_pair(found->template move<src, dest>(key, value), reinterpret_cast<List_Node *>(0x1));
        else
          return std::make_pair(std::make_tuple(std::tuple_element_t<src, typename SUBTRIE::Types>::Result::Failed_Removal, SUBTRIE::Create_Invalid(), KEY()), reinterpret_cast<List_Node *>(0x1));
      }

      const Singleton_Node<KEY, SUBTRIE> * find(const KEY &key) const {
        for (List_Node * head = const_cast<List_Node *>(this); head; head = head->next) {
          if (PRED()(head->snode->key, key))
            return head->snode;
        }
        return nullptr;
      }

      List_Node * erased(const Singleton_Node<KEY, SUBTRIE> * const snode) const {
        List_Node * new_head = nullptr;
        List_Node * new_tail = nullptr;
        for (List_Node * old_head = const_cast<List_Node *>(this); old_head; old_head = old_head->next) {
          if (old_head->snode == snode) {
            if (old_head->next) {
              if (!old_head->next->try_increment_refs()) {
                if (new_head)
                  delete new_head;
                return reinterpret_cast<List_Node *>(0x1);
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
                delete new_head;
              return reinterpret_cast<List_Node *>(0x1);
            }
            if (new_head)
              new_head = new List_Node(old_head->snode, new_head);
            else {
              new_head = new List_Node(old_head->snode, nullptr);
              new_tail = new_head;
            }
          }
        }
        return new_head;
      }

      Singleton_Node<KEY, SUBTRIE> * const snode;
      List_Node * next = nullptr;
    };
  }

  template <typename KEY, typename SUBTRIE, typename HASH, typename PRED, typename FLAG_TYPE>
  class Ctrie_NS_S2 {
    Ctrie_NS_S2(const Ctrie_NS_S2 &) = delete;
    Ctrie_NS_S2 operator=(const Ctrie_NS_S2 &) = delete;

  public:
    typedef KEY Key;
    typedef SUBTRIE Subtrie;
    typedef HASH Hash;
    typedef PRED Pred;

    typedef decltype(Hash()(Key())) Hash_Value;
    typedef FLAG_TYPE Flag_Type;

    typedef Ctrie_NS_S2_Internal::Node Node;
    typedef Ctrie_NS_S2_Internal::Main_Node MNode;
    typedef Ctrie_NS_S2_Internal::Singleton_Node<Key, Subtrie> SNode;
    typedef Ctrie_NS_S2_Internal::ICtrie_Node<Key, Subtrie, Hash, Pred, Flag_Type> CNode;
    typedef Ctrie_NS_S2_Internal::List_Node<Key, Subtrie, Pred> LNode;
    typedef Ctrie_NS_S2_Internal::Branch Branch;
    typedef Ctrie_NS_S2_Internal::Tomb_Node TNode;
    typedef Ctrie_NS_S2_Internal::Indirection_Node<Key, Subtrie> INode;

    Ctrie_NS_S2() = default;

    ~Ctrie_NS_S2() {
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

    template <size_t index>
    auto insert(const Key &key, const typename std::tuple_element_t<index, typename Subtrie::Types>::Key &value) {
      const Hash_Value hash_value = Hash()(key);
      for (;;) {
        INode * const root = Read_Root();
        const auto tuple_value = iinsert<index>(root, key, value, hash_value, 0, nullptr);
        if (std::get<0>(tuple_value) != std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart)
          return tuple_value;
      }
    }

    template <size_t index>
    auto insert_ip(const Key &key, const typename std::tuple_element_t<index, typename Subtrie::Types>::Key &value) {
      const Hash_Value hash_value = Hash()(key);
      for (;;) {
        INode * const root = Read_Root();
        const auto tuple_value = iinsert_ip<index>(root, key, value, hash_value, 0, nullptr);
        if (std::get<0>(tuple_value) != std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart)
          return tuple_value;
      }
    }

    template <size_t if_present, size_t regardless>
    auto insert_ip_xp(const Key &key, const typename std::tuple_element_t<if_present, typename Subtrie::Types>::Key &value) {
      const Hash_Value hash_value = Hash()(key);
      for (;;) {
        INode * const root = Read_Root();
        const auto tuple_value = iinsert_ip_xp<if_present, regardless>(root, key, value, hash_value, 0, nullptr);
        if (std::get<0>(tuple_value) != std::tuple_element_t<if_present, typename Subtrie::Types>::Result::Restart)
          return tuple_value;
      }
    }

    template <size_t index>
    auto erase(const Key &key, const typename std::tuple_element_t<index, typename Subtrie::Types>::Key &value) {
      const Hash_Value hash_value = Hash()(key);
      for (;;) {
        INode * const root = Read_Root();
        const auto tuple_value = ierase<index>(root, key, value, hash_value, 0, nullptr);
        if (std::get<0>(tuple_value) != std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart)
          return tuple_value;
      }
    }

    template <size_t index>
    auto erase_ip(const Key &key, const typename std::tuple_element_t<index, typename Subtrie::Types>::Key &value) {
      const Hash_Value hash_value = Hash()(key);
      for (;;) {
        INode * const root = Read_Root();
        const auto tuple_value = ierase_ip<index>(root, key, value, hash_value, 0, nullptr);
        if (std::get<0>(tuple_value) != std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart)
          return tuple_value;
      }
    }

    template <size_t if_present, size_t regardless>
    auto erase_ip_xp(const Key &key, const typename std::tuple_element_t<if_present, typename Subtrie::Types>::Key &value) {
      const Hash_Value hash_value = Hash()(key);
      for (;;) {
        INode * const root = Read_Root();
        const auto tuple_value = ierase_ip_xp<if_present, regardless>(root, key, value, hash_value, 0, nullptr);
        if (std::get<0>(tuple_value) != std::tuple_element_t<if_present, typename Subtrie::Types>::Result::Restart)
          return tuple_value;
      }
    }

    template <size_t src, size_t dest>
    auto move(const Key &key, const typename std::tuple_element_t<src, typename Subtrie::Types>::Key &value) {
      const Hash_Value hash_value = Hash()(key);
      for (;;) {
        INode * const root = Read_Root();
        const auto tuple_value = imove<src, dest>(root, key, value, hash_value, 0, nullptr);
        if (std::get<0>(tuple_value) != std::tuple_element_t<src, typename Subtrie::Types>::Result::Restart)
          return tuple_value;
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

    template <size_t index>
    auto iinsert(
      INode * inode,
      const Key &key,
      const typename std::tuple_element_t<index, typename Subtrie::Types>::Key &value,
      const Hash_Value &hash_value,
      size_t level,
      INode * parent)
    {
      for (;;) {
        const auto inode_main = CAS_Read(inode);
        if (!inode_main)
          return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
        if (auto cnode = dynamic_cast<CNode *>(inode_main)) {
          const auto[flag, pos] = cnode->flagpos(hash_value, level);
          Branch * branch = cnode->get_bmp() & flag ? cnode->at(pos) : nullptr;
          if (!branch) {
            const auto[tuple_value, new_snode] = SNode::template Create_Insert<index>(key, value);
            if (!new_snode)
              return tuple_value;
            const auto new_cnode = cnode->inserted(pos, flag, new_snode);
            if (!new_cnode)
              return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
            if (CAS_del(inode, inode_main, new_cnode))
              return tuple_value;
            else
              return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
          }
          else {
            if (const auto inode_next = dynamic_cast<INode *>(branch)) {
              if (deepen(inode, inode_main, level, parent, cnode, pos, inode_next))
                continue;
              else
                return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
            }
            else if (const auto snode = dynamic_cast<SNode *>(branch)) {
              if (Pred()(snode->key, key)) {
                const auto tuple_value = snode->template insert<index>(key, value);
                return post_op<index>(inode, level, tuple_value);
              }
              else {
                const Hash_Value snode_hash = Hash()(snode->key);
                if (snode_hash != hash_value) {
                  if (!snode->try_increment_refs())
                    return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
                  const auto[tuple_value, new_snode] = SNode::template Create_Insert<index>(key, value);
                  if (!new_snode)
                    return tuple_value;
                  const auto new_cnode = cnode->updated(pos, flag, new INode(CNode::Create(snode, snode_hash, new_snode, hash_value, level + CNode::W)));
                  if (!new_cnode)
                    return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
                  if (CAS_del(inode, inode_main, new_cnode))
                    return tuple_value;
                  else
                    return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
                }
                else {
                  if (!snode->try_increment_refs())
                    return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
                  const auto[tuple_value, new_snode] = SNode::template Create_Insert<index>(key, value);
                  if (!new_snode)
                    return tuple_value;
                  const auto new_cnode = cnode->updated(pos, flag, new INode(new LNode(new_snode, new LNode(snode))));
                  if (!new_cnode)
                    return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
                  if (CAS_del(inode, inode_main, new_cnode))
                    return tuple_value;
                  else
                    return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
                }
              }
            }
            else
              abort();
          }
        }
        else if (const auto tnode = dynamic_cast<TNode *>(inode_main)) {
          clean(parent, level - CNode::W);
          return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
        }
        else if (auto lnode = dynamic_cast<LNode *>(inode_main)) {
          const Hash_Value lnode_hash = Hash()(lnode->snode->key);
          if (lnode_hash != hash_value) {
            if (!lnode->try_increment_refs())
              return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
            const auto[tuple_value, new_snode] = SNode::template Create_Insert<index>(key, value);
            if (!new_snode)
              return tuple_value;
            const auto new_cnode = CNode::Create(new_snode, hash_value, new INode(lnode), lnode_hash, level);
            if (!new_cnode)
              return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
            if (CAS_del(inode, inode_main, new_cnode))
              return tuple_value;
            else
              return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
          }
          else {
            const auto[tuple_value, new_lnode] = lnode->template insert<index>(key, value);
            if (uintptr_t(new_lnode) == 0x1)
              return post_op<index>(inode, level, tuple_value);
            if (std::get<0>(tuple_value) == std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart)
              return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
            const auto lnode_ptr_part = reinterpret_cast<LNode *>(uintptr_t(new_lnode) & ~uintptr_t(0x2));
            MNode * replacement;
            if (lnode_ptr_part->next)
              replacement = lnode_ptr_part;
            else {
              [[maybe_unused]] const auto refs = lnode_ptr_part->snode->try_increment_refs();
              assert(refs);
              replacement = new TNode(lnode_ptr_part->snode);
              lnode_ptr_part->decrement_refs();
            }
            if (CAS_dec(inode, inode_main, replacement) || (uintptr_t(new_lnode) & 0x2))
              return tuple_value;
            else
              return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
          }
        }
      }
    }

    template <size_t index>
    auto iinsert_ip(
      INode * inode,
      const Key &key,
      const typename std::tuple_element_t<index, typename Subtrie::Types>::Key &value,
      const Hash_Value &hash_value,
      size_t level,
      INode * parent)
    {
      for (;;) {
        const auto inode_main = CAS_Read(inode);
        if (!inode_main)
          return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
        if (auto cnode = dynamic_cast<CNode *>(inode_main)) {
          const auto[flag, pos] = cnode->flagpos(hash_value, level);
          Branch * branch = cnode->get_bmp() & flag ? cnode->at(pos) : nullptr;
          if (!branch)
            return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Not_Present, typename Subtrie::Create_Invalid(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
          else {
            if (const auto inode_next = dynamic_cast<INode *>(branch)) {
              if (deepen(inode, inode_main, level, parent, cnode, pos, inode_next))
                continue;
              else
                return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
            }
            else if (const auto snode = dynamic_cast<SNode *>(branch)) {
              if (Pred()(snode->key, key)) {
                const auto tuple_value = snode->template insert_ip<index>(key, value);
                return post_op<index>(inode, level, tuple_value);
              }
              else
                return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Not_Present, typename Subtrie::Create_Invalid(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
            }
            else
              abort();
          }
        }
        else if (const auto tnode = dynamic_cast<TNode *>(inode_main)) {
          clean(parent, level - CNode::W);
          return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
        }
        else if (auto lnode = dynamic_cast<LNode *>(inode_main)) {
          const Hash_Value lnode_hash = Hash()(lnode->snode->key);
          if (lnode_hash != hash_value)
            return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Not_Present, typename Subtrie::Create_Invalid(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
          else {
            const auto[tuple_value, new_lnode] = lnode->template insert_ip<index>(key, value);
            if (uintptr_t(new_lnode) == 0x1)
              return post_op<index>(inode, level, tuple_value);
            if (std::get<0>(tuple_value) == std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart)
              return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
            const auto lnode_ptr_part = reinterpret_cast<LNode *>(uintptr_t(new_lnode) & ~uintptr_t(0x2));
            MNode * replacement;
            if (lnode_ptr_part->next)
              replacement = lnode_ptr_part;
            else {
              [[maybe_unused]] const auto refs = lnode_ptr_part->snode->try_increment_refs();
              assert(refs);
              replacement = new TNode(lnode_ptr_part->snode);
              lnode_ptr_part->decrement_refs();
            }
            if (CAS_dec(inode, inode_main, replacement) || (uintptr_t(new_lnode) & 0x2))
              return tuple_value;
            else
              return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
          }
        }
      }
    }

    template <size_t if_present, size_t regardless>
    auto iinsert_ip_xp(
      INode * inode,
      const Key &key,
      const typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key &value,
      const Hash_Value &hash_value,
      size_t level,
      INode * parent)
    {
      for (;;) {
        const auto inode_main = CAS_Read(inode);
        if (!inode_main)
          return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
        if (auto cnode = dynamic_cast<CNode *>(inode_main)) {
          const auto[flag, pos] = cnode->flagpos(hash_value, level);
          Branch * branch = cnode->get_bmp() & flag ? cnode->at(pos) : nullptr;
          if (!branch) {
            const auto[tuple_value, new_snode] = SNode::template Create_Insert<regardless>(key, value);
            if (!new_snode)
              return tuple_value;
            const auto new_cnode = cnode->inserted(pos, flag, new_snode);
            if (!new_cnode)
              return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
            if (CAS_del(inode, inode_main, new_cnode))
              return tuple_value;
            else
              return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
          }
          else {
            if (const auto inode_next = dynamic_cast<INode *>(branch)) {
              if (deepen(inode, inode_main, level, parent, cnode, pos, inode_next))
                continue;
              else
                return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
            }
            else if (const auto snode = dynamic_cast<SNode *>(branch)) {
              if (Pred()(snode->key, key)) {
                const auto tuple_value = snode->template insert_ip_xp<if_present, regardless>(key, value);
                return post_op<regardless>(inode, level, tuple_value);
              }
              else {
                const Hash_Value snode_hash = Hash()(snode->key);
                if (snode_hash != hash_value) {
                  if (!snode->try_increment_refs())
                    return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
                  const auto[tuple_value, new_snode] = SNode::template Create_Insert<regardless>(key, value);
                  if (!new_snode)
                    return tuple_value;
                  const auto new_cnode = cnode->updated(pos, flag, new INode(CNode::Create(snode, snode_hash, new_snode, hash_value, level + CNode::W)));
                  if (!new_cnode)
                    return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
                  if (CAS_del(inode, inode_main, new_cnode))
                    return tuple_value;
                  else
                    return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
                }
                else {
                  if (!snode->try_increment_refs())
                    return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
                  const auto[tuple_value, new_snode] = SNode::template Create_Insert<regardless>(key, value);
                  if (!new_snode)
                    return tuple_value;
                  const auto new_cnode = cnode->updated(pos, flag, new INode(new LNode(new_snode, new LNode(snode))));
                  if (!new_cnode)
                    return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
                  if (CAS_del(inode, inode_main, new_cnode))
                    return tuple_value;
                  else
                    return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
                }
              }
            }
            else
              abort();
          }
        }
        else if (const auto tnode = dynamic_cast<TNode *>(inode_main)) {
          clean(parent, level - CNode::W);
          return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
        }
        else if (auto lnode = dynamic_cast<LNode *>(inode_main)) {
          const Hash_Value lnode_hash = Hash()(lnode->snode->key);
          if (lnode_hash != hash_value) {
            if (!lnode->try_increment_refs())
              return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
            const auto[tuple_value, new_snode] = SNode::template Create_Insert<regardless>(key, value);
            if (!new_snode)
              return tuple_value;
            const auto new_cnode = CNode::Create(new_snode, hash_value, new INode(lnode), lnode_hash, level);
            if (!new_cnode)
              return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
            if (CAS_del(inode, inode_main, new_cnode))
              return tuple_value;
            else
              return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
          }
          else {
            const auto[tuple_value, new_lnode] = lnode->template insert_ip_xp<if_present, regardless>(key, value);
            if (uintptr_t(new_lnode) == 0x1)
              return post_op<regardless>(inode, level, tuple_value);
            if (std::get<0>(tuple_value) == std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart)
              return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
            const auto lnode_ptr_part = reinterpret_cast<LNode *>(uintptr_t(new_lnode) & ~uintptr_t(0x2));
            MNode * replacement;
            if (lnode_ptr_part->next)
              replacement = lnode_ptr_part;
            else {
              [[maybe_unused]] const auto refs = lnode_ptr_part->snode->try_increment_refs();
              assert(refs);
              replacement = new TNode(lnode_ptr_part->snode);
              lnode_ptr_part->decrement_refs();
            }
            if (CAS_dec(inode, inode_main, replacement) || (uintptr_t(new_lnode) & 0x2))
              return tuple_value;
            else
              return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
          }
        }
      }
    }

    template <size_t index>
    auto ierase(
      INode * inode,
      const Key &key,
      const typename std::tuple_element_t<index, typename Subtrie::Types>::Key &value,
      const Hash_Value &hash_value,
      size_t level,
      INode * parent)
    {
      for (;;) {
        const auto inode_main = CAS_Read(inode);
        if (!inode_main)
          return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
        if (auto cnode = dynamic_cast<CNode *>(inode_main)) {
          const auto[flag, pos] = cnode->flagpos(hash_value, level);
          Branch * branch = cnode->get_bmp() & flag ? cnode->at(pos) : nullptr;
          if (!branch) {
            const auto[tuple_value, new_snode] = SNode::template Create_Erase<index>(key, value);
            if (!new_snode)
              return tuple_value;
            const auto new_cnode = cnode->inserted(pos, flag, new_snode);
            if (!new_cnode)
              return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
            if (CAS_del(inode, inode_main, new_cnode))
              return tuple_value;
            else
              return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
          }
          else {
            if (const auto inode_next = dynamic_cast<INode *>(branch)) {
              if (deepen(inode, inode_main, level, parent, cnode, pos, inode_next))
                continue;
              else
                return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
            }
            else if (const auto snode = dynamic_cast<SNode *>(branch)) {
              if (Pred()(snode->key, key)) {
                const auto tuple_value = snode->template erase<index>(key, value);
                return post_op<index>(inode, level, tuple_value);
              }
              else {
                const Hash_Value snode_hash = Hash()(snode->key);
                if (snode_hash != hash_value) {
                  if (!snode->try_increment_refs())
                    return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
                  const auto[tuple_value, new_snode] = SNode::template Create_Erase<index>(key, value);
                  if (!new_snode)
                    return tuple_value;
                  const auto new_cnode = cnode->updated(pos, flag, new INode(CNode::Create(snode, snode_hash, new_snode, hash_value, level + CNode::W)));
                  if (!new_cnode)
                    return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
                  if (CAS_del(inode, inode_main, new_cnode))
                    return tuple_value;
                  else
                    return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
                }
                else {
                  if (!snode->try_increment_refs())
                    return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
                  const auto[tuple_value, new_snode] = SNode::template Create_Erase<index>(key, value);
                  if (!new_snode)
                    return tuple_value;
                  const auto new_cnode = cnode->updated(pos, flag, new INode(new LNode(new_snode, new LNode(snode))));
                  if (!new_cnode)
                    return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
                  if (CAS_del(inode, inode_main, new_cnode))
                    return tuple_value;
                  else
                    return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
                }
              }
            }
            else
              abort();
          }
        }
        else if (const auto tnode = dynamic_cast<TNode *>(inode_main)) {
          clean(parent, level - CNode::W);
          return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
        }
        else if (auto lnode = dynamic_cast<LNode *>(inode_main)) {
          const Hash_Value lnode_hash = Hash()(lnode->snode->key);
          if (lnode_hash != hash_value) {
            if (!lnode->try_increment_refs())
              return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
            const auto[tuple_value, new_snode] = SNode::template Create_Erase<index>(key, value);
            if (!new_snode)
              return tuple_value;
            const auto new_cnode = CNode::Create(new_snode, hash_value, new INode(lnode), lnode_hash, level);
            if (!new_cnode)
              return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
            if (CAS_del(inode, inode_main, new_cnode))
              return tuple_value;
            else
              return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
          }
          else {
            const auto[tuple_value, new_lnode] = lnode->template erase<index>(key, value);
            if (uintptr_t(new_lnode) == 0x1)
              return post_op<index>(inode, level, tuple_value);
            if (std::get<0>(tuple_value) == std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart)
              return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
            const auto lnode_ptr_part = reinterpret_cast<LNode *>(uintptr_t(new_lnode) & ~uintptr_t(0x2));
            MNode * replacement;
            if (lnode_ptr_part->next)
              replacement = lnode_ptr_part;
            else {
              [[maybe_unused]] const auto refs = lnode_ptr_part->snode->try_increment_refs();
              assert(refs);
              replacement = new TNode(lnode_ptr_part->snode);
              lnode_ptr_part->decrement_refs();
            }
            if (CAS_dec(inode, inode_main, replacement) || (uintptr_t(new_lnode) & 0x2))
              return tuple_value;
            else
              return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
          }
        }
      }
    }

    template <size_t index>
    auto ierase_ip(
      INode * inode,
      const Key &key,
      const typename std::tuple_element_t<index, typename Subtrie::Types>::Key &value,
      const Hash_Value &hash_value,
      size_t level,
      INode * parent)
    {
      for (;;) {
        const auto inode_main = CAS_Read(inode);
        if (!inode_main)
          return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
        if (auto cnode = dynamic_cast<CNode *>(inode_main)) {
          const auto[flag, pos] = cnode->flagpos(hash_value, level);
          Branch * branch = cnode->get_bmp() & flag ? cnode->at(pos) : nullptr;
          if (!branch)
            return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Not_Present, typename Subtrie::Create_Invalid(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
          else {
            if (const auto inode_next = dynamic_cast<INode *>(branch)) {
              if (deepen(inode, inode_main, level, parent, cnode, pos, inode_next))
                continue;
              else
                return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
            }
            else if (const auto snode = dynamic_cast<SNode *>(branch)) {
              if (Pred()(snode->key, key)) {
                const auto tuple_value = snode->template erase_ip<index>(key, value);
                return post_op<index>(inode, level, tuple_value);
              }
              else
                return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Not_Present, typename Subtrie::Create_Invalid(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
            }
            else
              abort();
          }
        }
        else if (const auto tnode = dynamic_cast<TNode *>(inode_main)) {
          clean(parent, level - CNode::W);
          return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
        }
        else if (auto lnode = dynamic_cast<LNode *>(inode_main)) {
          const Hash_Value lnode_hash = Hash()(lnode->snode->key);
          if (lnode_hash != hash_value)
            return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Not_Present, typename Subtrie::Create_Invalid(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
          else {
            const auto[tuple_value, new_lnode] = lnode->template erase_ip<index>(key, value);
            if (uintptr_t(new_lnode) == 0x1)
              return post_op<index>(inode, level, tuple_value);
            if (std::get<0>(tuple_value) == std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart)
              return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
            const auto lnode_ptr_part = reinterpret_cast<LNode *>(uintptr_t(new_lnode) & ~uintptr_t(0x2));
            MNode * replacement;
            if (lnode_ptr_part->next)
              replacement = lnode_ptr_part;
            else {
              [[maybe_unused]] const auto refs = lnode_ptr_part->snode->try_increment_refs();
              assert(refs);
              replacement = new TNode(lnode_ptr_part->snode);
              lnode_ptr_part->decrement_refs();
            }
            if (CAS_dec(inode, inode_main, replacement) || (uintptr_t(new_lnode) & 0x2))
              return tuple_value;
            else
              return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
          }
        }
      }
    }

    template <size_t if_present, size_t regardless>
    auto ierase_ip_xp(
      INode * inode,
      const Key &key,
      const typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key &value,
      const Hash_Value &hash_value,
      size_t level,
      INode * parent)
    {
      for (;;) {
        const auto inode_main = CAS_Read(inode);
        if (!inode_main)
          return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
        if (auto cnode = dynamic_cast<CNode *>(inode_main)) {
          const auto[flag, pos] = cnode->flagpos(hash_value, level);
          Branch * branch = cnode->get_bmp() & flag ? cnode->at(pos) : nullptr;
          if (!branch) {
            const auto[tuple_value, new_snode] = SNode::template Create_Erase<regardless>(key, value);
            if (!new_snode)
              return tuple_value;
            const auto new_cnode = cnode->inserted(pos, flag, new_snode);
            if (!new_cnode)
              return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
            if (CAS_del(inode, inode_main, new_cnode))
              return tuple_value;
            else
              return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
          }
          else {
            if (const auto inode_next = dynamic_cast<INode *>(branch)) {
              if (deepen(inode, inode_main, level, parent, cnode, pos, inode_next))
                continue;
              else
                return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
            }
            else if (const auto snode = dynamic_cast<SNode *>(branch)) {
              if (Pred()(snode->key, key)) {
                const auto tuple_value = snode->template erase_ip_xp<if_present, regardless>(key, value);
                return post_op<regardless>(inode, level, tuple_value);
              }
              else {
                const Hash_Value snode_hash = Hash()(snode->key);
                if (snode_hash != hash_value) {
                  if (!snode->try_increment_refs())
                    return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
                  const auto[tuple_value, new_snode] = SNode::template Create_Erase<regardless>(key, value);
                  if (!new_snode)
                    return tuple_value;
                  const auto new_cnode = cnode->updated(pos, flag, new INode(CNode::Create(snode, snode_hash, new_snode, hash_value, level + CNode::W)));
                  if (!new_cnode)
                    return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
                  if (CAS_del(inode, inode_main, new_cnode))
                    return tuple_value;
                  else
                    return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
                }
                else {
                  if (!snode->try_increment_refs())
                    return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
                  const auto[tuple_value, new_snode] = SNode::template Create_Erase<regardless>(key, value);
                  if (!new_snode)
                    return tuple_value;
                  const auto new_cnode = cnode->updated(pos, flag, new INode(new LNode(new_snode, new LNode(snode))));
                  if (!new_cnode)
                    return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
                  if (CAS_del(inode, inode_main, new_cnode))
                    return tuple_value;
                  else
                    return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
                }
              }
            }
            else
              abort();
          }
        }
        else if (const auto tnode = dynamic_cast<TNode *>(inode_main)) {
          clean(parent, level - CNode::W);
          return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
        }
        else if (auto lnode = dynamic_cast<LNode *>(inode_main)) {
          const Hash_Value lnode_hash = Hash()(lnode->snode->key);
          if (lnode_hash != hash_value) {
            if (!lnode->try_increment_refs())
              return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
            const auto[tuple_value, new_snode] = SNode::template Create_Erase<regardless>(key, value);
            if (!new_snode)
              return tuple_value;
            const auto new_cnode = CNode::Create(new_snode, hash_value, new INode(lnode), lnode_hash, level);
            if (!new_cnode)
              return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
            if (CAS_del(inode, inode_main, new_cnode))
              return tuple_value;
            else
              return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
          }
          else {
            const auto[tuple_value, new_lnode] = lnode->template erase_ip_xp<if_present, regardless>(key, value);
            if (uintptr_t(new_lnode) == 0x1)
              return post_op<regardless>(inode, level, tuple_value);
            if (std::get<0>(tuple_value) == std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart)
              return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
            const auto lnode_ptr_part = reinterpret_cast<LNode *>(uintptr_t(new_lnode) & ~uintptr_t(0x2));
            MNode * replacement;
            if (lnode_ptr_part->next)
              replacement = lnode_ptr_part;
            else {
              [[maybe_unused]] const auto refs = lnode_ptr_part->snode->try_increment_refs();
              assert(refs);
              replacement = new TNode(lnode_ptr_part->snode);
              lnode_ptr_part->decrement_refs();
            }
            if (CAS_dec(inode, inode_main, replacement) || (uintptr_t(new_lnode) & 0x2))
              return tuple_value;
            else
              return std::make_tuple(std::tuple_element_t<regardless, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<regardless, typename Subtrie::Types>::Key());
          }
        }
      }
    }

    template <size_t src, size_t dest>
    auto imove(
      INode * inode,
      const Key &key,
      const typename std::tuple_element_t<src, typename Subtrie::Types>::Key &value,
      const Hash_Value &hash_value,
      size_t level,
      INode * parent)
    {
      for (;;) {
        const auto inode_main = CAS_Read(inode);
        if (!inode_main)
          return std::make_tuple(std::tuple_element_t<src, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<src, typename Subtrie::Types>::Key());
        if (auto cnode = dynamic_cast<CNode *>(inode_main)) {
          const auto[flag, pos] = cnode->flagpos(hash_value, level);
          Branch * branch = cnode->get_bmp() & flag ? cnode->at(pos) : nullptr;
          if (!branch)
            return std::make_tuple(std::tuple_element_t<src, typename Subtrie::Types>::Result::Not_Present, typename Subtrie::Create_Invalid(), typename std::tuple_element_t<src, typename Subtrie::Types>::Key());
          else {
            if (const auto inode_next = dynamic_cast<INode *>(branch)) {
              if (deepen(inode, inode_main, level, parent, cnode, pos, inode_next))
                continue;
              else
                return std::make_tuple(std::tuple_element_t<src, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<src, typename Subtrie::Types>::Key());
            }
            else if (const auto snode = dynamic_cast<SNode *>(branch)) {
              if (Pred()(snode->key, key)) {
                const auto tuple_value = snode->template move<src, dest>(key, value);
                return post_op<src>(inode, level, tuple_value);
              }
              else
                return std::make_tuple(std::tuple_element_t<src, typename Subtrie::Types>::Result::Not_Present, typename Subtrie::Create_Invalid(), typename std::tuple_element_t<src, typename Subtrie::Types>::Key());
            }
            else
              abort();
          }
        }
        else if (const auto tnode = dynamic_cast<TNode *>(inode_main)) {
          clean(parent, level - CNode::W);
          return std::make_tuple(std::tuple_element_t<src, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<src, typename Subtrie::Types>::Key());
        }
        else if (auto lnode = dynamic_cast<LNode *>(inode_main)) {
          const Hash_Value lnode_hash = Hash()(lnode->snode->key);
          if (lnode_hash != hash_value)
            return std::make_tuple(std::tuple_element_t<src, typename Subtrie::Types>::Result::Not_Present, typename Subtrie::Create_Invalid(), typename std::tuple_element_t<src, typename Subtrie::Types>::Key());
          else {
            const auto[tuple_value, new_lnode] = lnode->template move<src, dest>(key, value);
            if (uintptr_t(new_lnode) == 0x1)
              return post_op<src>(inode, level, tuple_value);
            if (std::get<0>(tuple_value) == std::tuple_element_t<src, typename Subtrie::Types>::Result::Restart)
              return std::make_tuple(std::tuple_element_t<src, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<src, typename Subtrie::Types>::Key());
            const auto lnode_ptr_part = reinterpret_cast<LNode *>(uintptr_t(new_lnode) & ~uintptr_t(0x2));
            MNode * replacement;
            if (lnode_ptr_part->next)
              replacement = lnode_ptr_part;
            else {
              [[maybe_unused]] const auto refs = lnode_ptr_part->snode->try_increment_refs();
              assert(refs);
              replacement = new TNode(lnode_ptr_part->snode);
              lnode_ptr_part->decrement_refs();
            }
            if (CAS_dec(inode, inode_main, replacement) || (uintptr_t(new_lnode) & 0x2))
              return tuple_value;
            else
              return std::make_tuple(std::tuple_element_t<src, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<src, typename Subtrie::Types>::Key());
          }
        }
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

    template <size_t index>
    auto post_op(INode * const inode, const size_t level, const std::tuple<typename std::tuple_element_t<index, typename Subtrie::Types>::Result, typename Subtrie::Snapshot, typename std::tuple_element_t<index, typename Subtrie::Types>::Key> &tuple_value) {
      if (std::get<0>(tuple_value) == std::tuple_element_t<index, typename Subtrie::Types>::Result::Invalid_SHT) {
        clean(inode, level);
        return std::make_tuple(std::tuple_element_t<index, typename Subtrie::Types>::Result::Restart, typename Subtrie::Snapshot(), typename std::tuple_element_t<index, typename Subtrie::Types>::Key());
      }
      if (!std::get<1>(tuple_value).valid())
        clean(inode, level);
      return tuple_value;
    }

    void clean(INode * const inode, const size_t level) const {
      const auto inode_main = CAS_Read(inode);
      if (const auto cnode = dynamic_cast<CNode *>(inode_main)) {
        if (MNode * const compressed = cnode->to_compressed(level))
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
              const MNode * new_mnode;
              if (tnode->branch) {
                MNode * const resurrected = inode->resurrect();
                if (!resurrected)
                  continue;
                new_mnode = cnode->updated(pos, flag, resurrected)->to_contracted(level);
              }
              else
                new_mnode = cnode->erased(pos, flag)->to_contracted(level);
              if (!new_mnode)
                continue;
              if (!CAS_del(parent, parent_main, new_mnode))
                continue;
            }
          }
        }
      } while (false);
    }

    bool CAS_dec(INode * const inode, MNode * old_mnode, MNode * const new_mnode) const {
      return CAS_dec(inode->main, old_mnode, new_mnode, std::memory_order_release, std::memory_order_relaxed);
    }

    bool CAS_del(INode * const inode, MNode * old_mnode, MNode * const new_mnode) const {
      return CAS_del(inode->main, old_mnode, new_mnode, std::memory_order_release, std::memory_order_relaxed);
    }

  public:
    MNode * CAS_Read(const INode * const inode) const {
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
