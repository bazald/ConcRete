#ifndef ZENI_RETE_NODE_KEY_HPP
#define ZENI_RETE_NODE_KEY_HPP

#include "Zeni/Rete/Symbol.hpp"
#include "Zeni/Rete/Variable_Binding.hpp"

namespace Zeni::Rete {

  class Node_Key : std::enable_shared_from_this<Node_Key> {
    Node_Key(const Node_Key &) = delete;
    Node_Key & operator=(const Node_Key &) = delete;

  public:
    ZENI_RETE_LINKAGE Node_Key() {}

    virtual ~Node_Key() = 0;
  };

  class Node_Key_Symbol : public Node_Key {
    Node_Key_Symbol(const Node_Key_Symbol &) = delete;
    Node_Key_Symbol & operator=(const Node_Key_Symbol &) = delete;

  public:
    Node_Key_Symbol(const std::shared_ptr<const Symbol> symbol);

    const std::shared_ptr<const Symbol> symbol;
  };

  class Node_Key_Variable_Bindings : public Node_Key {
    Node_Key_Variable_Bindings(const Node_Key_Variable_Bindings &) = delete;
    Node_Key_Variable_Bindings & operator=(const Node_Key_Variable_Bindings &) = delete;

  public:
    Node_Key_Variable_Bindings(const std::shared_ptr<const Variable_Bindings> variable_bindings);

    const std::shared_ptr<const Variable_Bindings> variable_bindings;
  };

}

#endif
