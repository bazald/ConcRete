#include "Zeni/Rete/Internal/Node_Key.hpp"

namespace Zeni::Rete {

  Node_Key::~Node_Key()
  {
  }

  Node_Key_Symbol::Node_Key_Symbol(const std::shared_ptr<const Symbol> symbol_)
    : symbol(symbol_)
  {
  }

  Node_Key_Variable_Bindings::Node_Key_Variable_Bindings(const std::shared_ptr<const Variable_Bindings> variable_bindings_)
    : variable_bindings(variable_bindings_)
  {
  }

}
