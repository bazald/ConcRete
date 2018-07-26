#ifndef ZENI_CONCURRENCY_CTRIE_HPP
#define ZENI_CONCURRENCY_CTRIE_HPP

#include "Intrusive_Shared_Ptr.hpp"

#include <array>
#include <functional>

namespace Zeni::Concurrency {

  namespace Ctrie_Internal {
    template <typename INT_TYPE>
    constexpr static INT_TYPE tmp_hamming(const INT_TYPE value = std::numeric_limits<INT_TYPE>::max()) {
      return (value & 0x1) + ((value >> 1) ? tmp_hamming(value >> 1) : 0);
    }

    template <typename SUMMABLE_TYPE>
    constexpr static SUMMABLE_TYPE tmp_sum(const SUMMABLE_TYPE lhs, const SUMMABLE_TYPE rhs) {
      return lhs + rhs;
    }

    struct MainNode : public Enable_Intrusive_Sharing<MainNode> {};

    struct Branch : public Enable_Intrusive_Sharing<Branch> {};

    struct ICNode : public MainNode {
      static typename Intrusive_Shared_Ptr<ICNode>::Lock Create(const uintptr_t bmp) {
        static Factory factory;
        return factory.create(bmp);
      }

      virtual uintptr_t get_bmp() = 0;

    private:
      class Factory {
        Factory(const Factory &) = delete;
        Factory & operator=(const Factory &) = delete;

      public:
        inline Factory();

        typename Intrusive_Shared_Ptr<ICNode>::Lock create(const uintptr_t bmp) {
          return m_generator[bmp]();
        }

      private:
        std::array<std::function<typename Intrusive_Shared_Ptr<ICNode>::Lock()>, tmp_sum(tmp_hamming<uintptr_t>(), uintptr_t(1))> m_generator;
      };
    };

    template <uintptr_t BMP>
    struct CNode : public ICNode {
      uintptr_t get_bmp() override {
        return bmp;
      };

      static constexpr uintptr_t bmp = BMP;

      Intrusive_Shared_Ptr<Branch> branches[bmp];
    };

    template <uintptr_t IN = tmp_hamming<uintptr_t>()>
    struct CNode_Generator {
      static void Create(std::array<std::function<typename Intrusive_Shared_Ptr<ICNode>::Lock()>, tmp_sum(tmp_hamming<uintptr_t>(), uintptr_t(1))> &generator) {
        CNode_Generator<IN - 1>::Create(generator);

        generator[IN] = []()->typename Intrusive_Shared_Ptr<ICNode>::Lock {
          return new CNode<IN>;
        };
      }
    };

    template <>
    struct CNode_Generator<0> {
      static void Create(std::array<std::function<typename Intrusive_Shared_Ptr<ICNode>::Lock()>, tmp_sum(tmp_hamming<uintptr_t>(), uintptr_t(1))> &generator) {
        generator[0] = []()->typename Intrusive_Shared_Ptr<ICNode>::Lock {
          return new CNode<0>;
        };
      }
    };

    ICNode::Factory::Factory() {
      CNode_Generator<>::Create(m_generator);
    }

    template <typename KEY, typename TYPE>
    struct SNode : public Branch {
      KEY key;
      TYPE value;
    };

    template <typename KEY, typename TYPE>
    struct TNode : public MainNode {
      Intrusive_Shared_Ptr<SNode<KEY, TYPE>> snode;
    };

    template <typename KEY, typename TYPE>
    struct LNode : public TNode<KEY, TYPE> {
      Intrusive_Shared_Ptr<LNode> next;
    };

    template <typename GENERATION = std::uint64_t>
    struct INode : public Branch {
      Intrusive_Shared_Ptr<MainNode> main;
      GENERATION gen;
    };
  }

  template <typename KEY, typename TYPE, typename HASH = std::hash<TYPE>, typename GENERATION = std::uint64_t>
  class Ctrie {
    Ctrie(const Ctrie &) = delete;
    Ctrie & operator=(const Ctrie &) = delete;

  public:

    Ctrie() noexcept = default;

    ~Ctrie() {
      Ctrie_Internal::ICNode::Create(12);
    }

  private:
    Intrusive_Shared_Ptr<Ctrie_Internal::INode<>> m_root;
  };

}

#endif
