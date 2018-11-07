#ifndef ZENI_RETE_NODE_KEY_HPP
#define ZENI_RETE_NODE_KEY_HPP

#include "Zeni/Rete/Symbol.hpp"
#include "Zeni/Rete/Variable_Binding.hpp"

namespace Zeni::Rete {

  class Node_Key_Symbol;
  class Node_Key_Null;
  class Node_Key_01;
  class Node_Key_02;
  class Node_Key_12;
  class Node_Key_Variable_Bindings;

  class Node_Key : std::enable_shared_from_this<Node_Key> {
    Node_Key(const Node_Key &) = delete;
    Node_Key & operator=(const Node_Key &) = delete;

  public:
    ZENI_RETE_LINKAGE Node_Key() {}

    virtual size_t hash() const = 0;

    virtual bool operator==(const Node_Key &rhs) const = 0;

    virtual bool operator==(const Node_Key_Symbol &rhs) const;
    virtual bool operator==(const Node_Key_Null &rhs) const;
    virtual bool operator==(const Node_Key_01 &rhs) const;
    virtual bool operator==(const Node_Key_02 &rhs) const;
    virtual bool operator==(const Node_Key_12 &rhs) const;
    virtual bool operator==(const Node_Key_Variable_Bindings &rhs) const;
  };

  class Node_Key_Symbol : public Node_Key {
    Node_Key_Symbol(const Node_Key_Symbol &) = delete;
    Node_Key_Symbol & operator=(const Node_Key_Symbol &) = delete;

  public:
    Node_Key_Symbol(const std::shared_ptr<const Symbol> symbol);

    size_t hash() const override;
    bool operator==(const Node_Key &rhs) const override;
    bool operator==(const Node_Key_Symbol &rhs) const override;

    const std::shared_ptr<const Symbol> symbol;
  };

  class Node_Key_Null : public Node_Key {
    Node_Key_Null(const Node_Key_Null &) = delete;
    Node_Key_Null & operator=(const Node_Key_Null &) = delete;

  public:
    Node_Key_Null();

    size_t hash() const override;
    bool operator==(const Node_Key &rhs) const override;
    bool operator==(const Node_Key_Null &rhs) const override;
  };

  class Node_Key_01 : public Node_Key {
    Node_Key_01(const Node_Key_01 &) = delete;
    Node_Key_01 & operator=(const Node_Key_01 &) = delete;

  public:
    Node_Key_01();

    size_t hash() const override;
    bool operator==(const Node_Key &rhs) const override;
    bool operator==(const Node_Key_01 &rhs) const override;
  };

  class Node_Key_02 : public Node_Key {
    Node_Key_02(const Node_Key_02 &) = delete;
    Node_Key_02 & operator=(const Node_Key_02 &) = delete;

  public:
    Node_Key_02();

    size_t hash() const override;
    bool operator==(const Node_Key &rhs) const override;
    bool operator==(const Node_Key_02 &rhs) const override;
  };

  class Node_Key_12 : public Node_Key {
    Node_Key_12(const Node_Key_12 &) = delete;
    Node_Key_12 & operator=(const Node_Key_12 &) = delete;

  public:
    Node_Key_12();

    size_t hash() const override;
    bool operator==(const Node_Key &rhs) const override;
    bool operator==(const Node_Key_12 &rhs) const override;
  };

  class Node_Key_Variable_Bindings : public Node_Key {
    Node_Key_Variable_Bindings(const Node_Key_Variable_Bindings &) = delete;
    Node_Key_Variable_Bindings & operator=(const Node_Key_Variable_Bindings &) = delete;

  public:
    Node_Key_Variable_Bindings(const std::shared_ptr<const Variable_Bindings> variable_bindings);

    size_t hash() const override;
    bool operator==(const Node_Key &rhs) const override;
    bool operator==(const Node_Key_Variable_Bindings &rhs) const override;

    const std::shared_ptr<const Variable_Bindings> variable_bindings;
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
