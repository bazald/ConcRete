#ifndef ZENI_CONCURRENCY_CTRIE_HPP
#define ZENI_CONCURRENCY_CTRIE_HPP

#include "Intrusive_Shared_Ptr.hpp"

#include <array>
#include <functional>

namespace Zeni::Concurrency {

  namespace Ctrie_Internal {
    template <typename INT_TYPE>
    constexpr static INT_TYPE hamming(const INT_TYPE value = std::numeric_limits<INT_TYPE>::max()) {
      return (value & 0x1) + ((value >> 1) ? hamming(value >> 1) : 0);
    }

    template <typename SUMMABLE_TYPE>
    constexpr static SUMMABLE_TYPE sum(const SUMMABLE_TYPE lhs, const SUMMABLE_TYPE rhs) {
      return lhs + rhs;
    }

    struct MainNode : public Enable_Intrusive_Sharing<MainNode> {
    private:
      MainNode(const MainNode &) = delete;
      MainNode & operator=(const MainNode &) = delete;

    public:
      MainNode() = default;
    };

    struct Branch : public Enable_Intrusive_Sharing<Branch> {
    private:
      Branch(const Branch &) = delete;
      Branch & operator=(const Branch &) = delete;

    public:
      Branch() = default;
    };

    template <typename HASH_VALUE_TYPE>
    struct ICtrie_Node : public MainNode {
    private:
      ICtrie_Node(const ICtrie_Node &) = delete;
      ICtrie_Node & operator=(const ICtrie_Node &) = delete;

    protected:
      ICtrie_Node(const HASH_VALUE_TYPE bmp)
        : m_bmp(bmp)
      {
      }

    public:
      static ICtrie_Node * Create(const HASH_VALUE_TYPE bmp) {
        static Factory factory;
        return factory.create(bmp);
      }

      HASH_VALUE_TYPE get_bmp() const { return m_bmp; }

      virtual size_t get_hamming_value() const = 0;

    private:
      HASH_VALUE_TYPE m_bmp;

      class Factory {
        Factory(const Factory &) = delete;
        Factory & operator=(const Factory &) = delete;

      public:
        inline Factory();

        ICtrie_Node<HASH_VALUE_TYPE> * create(const HASH_VALUE_TYPE bmp) {
          return m_generator[bmp](bmp);
        }

      private:
        std::array<std::function<ICtrie_Node<HASH_VALUE_TYPE> *(const HASH_VALUE_TYPE bmp)>, sum(hamming<HASH_VALUE_TYPE>(), HASH_VALUE_TYPE(1))> m_generator;
      };
    };

    template <typename HASH_VALUE_TYPE, size_t HAMMING_VALUE>
    struct Ctrie_Node : public ICtrie_Node<HASH_VALUE_TYPE> {
    private:
      Ctrie_Node(const Ctrie_Node &) = delete;
      Ctrie_Node & operator=(const Ctrie_Node &) = delete;

    public:
      static const size_t hamming_value = HAMMING_VALUE;

      Ctrie_Node(const HASH_VALUE_TYPE bmp)
        : ICtrie_Node<HASH_VALUE_TYPE>(bmp)
      {
      }

      size_t get_hamming_value() const override {
        return hamming_value;
      };

      Intrusive_Shared_Ptr<Branch> branches[hamming_value];
    };

    template <typename HASH_VALUE_TYPE, size_t IN = hamming<HASH_VALUE_TYPE>()>
    struct Ctrie_Node_Generator {
      static void Create(std::array<std::function<ICtrie_Node<HASH_VALUE_TYPE> *(const HASH_VALUE_TYPE bmp)>, sum(hamming<HASH_VALUE_TYPE>(), HASH_VALUE_TYPE(1))> &generator) {
        Ctrie_Node_Generator<HASH_VALUE_TYPE, IN - 1>::Create(generator);

        generator[IN] = [](const HASH_VALUE_TYPE bmp)->ICtrie_Node<HASH_VALUE_TYPE> * {
          return new Ctrie_Node<HASH_VALUE_TYPE, IN>(bmp);
        };
      }
    };

    template <typename HASH_VALUE_TYPE>
    struct Ctrie_Node_Generator<HASH_VALUE_TYPE, 0> {
      static void Create(std::array<std::function<ICtrie_Node<HASH_VALUE_TYPE> *(const HASH_VALUE_TYPE bmp)>, sum(hamming<HASH_VALUE_TYPE>(), HASH_VALUE_TYPE(1))> &generator) {
        generator[0] = [](const HASH_VALUE_TYPE bmp)->ICtrie_Node<HASH_VALUE_TYPE> * {
          return new Ctrie_Node<HASH_VALUE_TYPE, 0>(bmp);
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

      KEY key;
      TYPE value;
    };

    template <typename KEY, typename TYPE>
    struct Tomb_Node : public MainNode {
    private:
      Tomb_Node(const Tomb_Node &) = delete;
      Tomb_Node  &operator=(const Tomb_Node &) = delete;

    public:
      Tomb_Node() = default;

      template <typename SINGLETON_NODE>
      Tomb_Node(SINGLETON_NODE &&snode_) : snode(std::forward<SINGLETON_NODE>(snode_)) {}

      Intrusive_Shared_Ptr<Singleton_Node<KEY, TYPE>> snode;
    };

    template <typename KEY, typename TYPE>
    struct List_Node : public Tomb_Node<KEY, TYPE> {
    private:
      List_Node(const List_Node &) = delete;
      List_Node & operator=(const List_Node &) = delete;

    public:
      List_Node() = default;

      template <typename SINGLETON_NODE, typename LIST_NODE>
      List_Node(SINGLETON_NODE &&snode_, LIST_NODE &&next_) : snode(std::forward<SINGLETON_NODE>(snode_)), next(std::forward<LIST_NODE>(next_)) {}

      Intrusive_Shared_Ptr<Singleton_Node<KEY, TYPE>> snode;
      Intrusive_Shared_Ptr<List_Node> next;
    };

    template <typename GENERATION = std::uint64_t>
    struct Indirection_Node : public Branch {
    private:
      Indirection_Node(const Indirection_Node &) = delete;
      Indirection_Node & operator=(const Indirection_Node &) = delete;

    public:
      Indirection_Node() : gen(GENERATION()) {}

      template <typename MAIN_NODE>
      Indirection_Node(MAIN_NODE &&main_, GENERATION &&gen_) : main(std::forward<MAIN_NODE>(main_)), gen(std::forward<GENERATION>(gen_)) {}

      Intrusive_Shared_Ptr<MainNode> main;
      GENERATION gen;
    };
  }

  template <typename KEY, typename TYPE, typename HASH = std::hash<TYPE>, typename GENERATION = std::uint64_t>
  class Ctrie {
    Ctrie(const Ctrie &) = delete;
    Ctrie & operator=(const Ctrie &) = delete;

  public:
    Ctrie() = default;

    Intrusive_Shared_Ptr<Ctrie_Internal::Singleton_Node<KEY, TYPE>> lookup(const KEY &key) {
      Intrusive_Shared_Ptr<Ctrie_Internal::Singleton_Node<KEY, TYPE>> found;
      for(;;) {
        Intrusive_Shared_Ptr<Ctrie_Internal::Indirection_Node<>> root = m_root;
        if (!ilookup(found, root, key, 0, nullptr))
          break;
      }
      return found;
    }

  private:
    bool ilookup(Intrusive_Shared_Ptr<Ctrie_Internal::Singleton_Node<KEY, TYPE>> &found,
      const Intrusive_Shared_Ptr<Ctrie_Internal::Indirection_Node<>> &inode,
      const KEY &key,
      const size_t level,
      const Intrusive_Shared_Ptr<Ctrie_Internal::Indirection_Node<>> &parent)
    {
      return false;
    }

    Intrusive_Shared_Ptr<Ctrie_Internal::Indirection_Node<>> m_root = new Ctrie_Internal::Indirection_Node<>(static_cast<Ctrie_Internal::MainNode *>(Ctrie_Internal::ICtrie_Node<uintptr_t>::Create(0x0)), 0);
  };

}

#endif
