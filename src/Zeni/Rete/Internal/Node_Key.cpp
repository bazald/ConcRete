#include "Zeni/Rete/Internal/Node_Key.hpp"

namespace Zeni::Rete {

  bool Node_Key::operator==(const Node_Key_Symbol &) const {
    return false;
  }

  bool Node_Key::operator==(const Node_Key_Null &) const {
    return false;
  }

  bool Node_Key::operator==(const Node_Key_01 &) const {
    return false;
  }

  bool Node_Key::operator==(const Node_Key_02 &) const {
    return false;
  }

  bool Node_Key::operator==(const Node_Key_12 &) const {
    return false;
  }

  bool Node_Key::operator==(const Node_Key_Variable_Bindings &) const {
    return false;
  }

  Node_Key_Symbol::Node_Key_Symbol(const std::shared_ptr<const Symbol> symbol_)
    : symbol(symbol_)
  {
  }

  size_t Node_Key_Symbol::hash() const {
    return symbol->hash();
  }

  bool Node_Key_Symbol::operator==(const Node_Key &rhs) const {
    return rhs == *this;
  }

  bool Node_Key_Symbol::operator==(const Node_Key_Symbol &rhs) const {
    return *symbol == *rhs.symbol;
  }

  Node_Key_Null::Node_Key_Null()
  {
  }

  size_t Node_Key_Null::hash() const {
    return std::hash<size_t>()(1);
  }

  bool Node_Key_Null::operator==(const Node_Key &) const {
    return true;
  }

  bool Node_Key_Null::operator==(const Node_Key_Null &) const {
    return true;
  }

  Node_Key_01::Node_Key_01()
  {
  }

  size_t Node_Key_01::hash() const {
    return std::hash<size_t>()(2);
  }

  bool Node_Key_01::operator==(const Node_Key &) const {
    return true;
  }

  bool Node_Key_01::operator==(const Node_Key_01 &) const {
    return true;
  }

  Node_Key_02::Node_Key_02()
  {
  }

  size_t Node_Key_02::hash() const {
    return std::hash<size_t>()(3);
  }

  bool Node_Key_02::operator==(const Node_Key &) const {
    return true;
  }

  bool Node_Key_02::operator==(const Node_Key_02 &) const {
    return true;
  }

  Node_Key_12::Node_Key_12()
  {
  }

  size_t Node_Key_12::hash() const {
    return std::hash<size_t>()(4);
  }

  bool Node_Key_12::operator==(const Node_Key &) const {
    return true;
  }

  bool Node_Key_12::operator==(const Node_Key_12 &) const {
    return true;
  }

  Node_Key_Variable_Bindings::Node_Key_Variable_Bindings(const std::shared_ptr<const Variable_Bindings> variable_bindings_)
    : variable_bindings(variable_bindings_)
  {
  }

  size_t Node_Key_Variable_Bindings::hash() const {
    return std::hash<Variable_Bindings>()(*variable_bindings);
  }

  bool Node_Key_Variable_Bindings::operator==(const Node_Key &rhs) const {
    return rhs == *this;
  }

  bool Node_Key_Variable_Bindings::operator==(const Node_Key_Variable_Bindings &rhs) const {
    return *variable_bindings == *rhs.variable_bindings;
  }

}
