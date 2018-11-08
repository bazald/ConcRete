#ifndef ZENI_RETE_NODE_KEY_HPP
#define ZENI_RETE_NODE_KEY_HPP

#include "Symbol.hpp"
#include "Variable_Binding.hpp"

namespace Zeni::Rete {

  class Node_Key_Symbol;
  class Node_Key_Null;
  class Node_Key_01;
  class Node_Key_02;
  class Node_Key_12;

  class Node_Key : std::enable_shared_from_this<Node_Key> {
    Node_Key(const Node_Key &) = delete;
    Node_Key & operator=(const Node_Key &) = delete;

  public:
    ZENI_RETE_LINKAGE Node_Key() {}

    ZENI_RETE_LINKAGE virtual size_t hash() const = 0;

    ZENI_RETE_LINKAGE virtual bool operator==(const Node_Key &rhs) const = 0;

    bool operator!=(const Node_Key &rhs) const { return !(*this == rhs); }

    ZENI_RETE_LINKAGE virtual bool operator==(const Node_Key_Symbol &rhs) const;
    ZENI_RETE_LINKAGE virtual bool operator==(const Node_Key_Null &rhs) const;
    ZENI_RETE_LINKAGE virtual bool operator==(const Node_Key_01 &rhs) const;
    ZENI_RETE_LINKAGE virtual bool operator==(const Node_Key_02 &rhs) const;
    ZENI_RETE_LINKAGE virtual bool operator==(const Node_Key_12 &rhs) const;
  };

  class Node_Key_Symbol : public Node_Key {
    Node_Key_Symbol(const Node_Key_Symbol &) = delete;
    Node_Key_Symbol & operator=(const Node_Key_Symbol &) = delete;

  public:
    ZENI_RETE_LINKAGE Node_Key_Symbol(const std::shared_ptr<const Symbol> symbol);

    ZENI_RETE_LINKAGE size_t hash() const override;
    ZENI_RETE_LINKAGE bool operator==(const Node_Key &rhs) const override;
    ZENI_RETE_LINKAGE bool operator==(const Node_Key_Symbol &rhs) const override;

    const std::shared_ptr<const Symbol> symbol;
  };

  class Node_Key_Null : public Node_Key {
    Node_Key_Null(const Node_Key_Null &) = delete;
    Node_Key_Null & operator=(const Node_Key_Null &) = delete;

  public:
    ZENI_RETE_LINKAGE Node_Key_Null();

    ZENI_RETE_LINKAGE size_t hash() const override;
    ZENI_RETE_LINKAGE bool operator==(const Node_Key &rhs) const override;
    ZENI_RETE_LINKAGE bool operator==(const Node_Key_Null &rhs) const override;
  };

  class Node_Key_01 : public Node_Key {
    Node_Key_01(const Node_Key_01 &) = delete;
    Node_Key_01 & operator=(const Node_Key_01 &) = delete;

  public:
    ZENI_RETE_LINKAGE Node_Key_01();

    ZENI_RETE_LINKAGE size_t hash() const override;
    ZENI_RETE_LINKAGE bool operator==(const Node_Key &rhs) const override;
    ZENI_RETE_LINKAGE bool operator==(const Node_Key_01 &rhs) const override;
  };

  class Node_Key_02 : public Node_Key {
    Node_Key_02(const Node_Key_02 &) = delete;
    Node_Key_02 & operator=(const Node_Key_02 &) = delete;

  public:
    ZENI_RETE_LINKAGE Node_Key_02();

    ZENI_RETE_LINKAGE size_t hash() const override;
    ZENI_RETE_LINKAGE bool operator==(const Node_Key &rhs) const override;
    ZENI_RETE_LINKAGE bool operator==(const Node_Key_02 &rhs) const override;
  };

  class Node_Key_12 : public Node_Key {
    Node_Key_12(const Node_Key_12 &) = delete;
    Node_Key_12 & operator=(const Node_Key_12 &) = delete;

  public:
    ZENI_RETE_LINKAGE Node_Key_12();

    ZENI_RETE_LINKAGE size_t hash() const override;
    ZENI_RETE_LINKAGE bool operator==(const Node_Key &rhs) const override;
    ZENI_RETE_LINKAGE bool operator==(const Node_Key_12 &rhs) const override;
  };

}

namespace std {
  template <> struct hash<Zeni::Rete::Node_Key> {
    size_t operator()(const Zeni::Rete::Node_Key &node_key) const {
      return node_key.hash();
    }
  };
}

#endif
