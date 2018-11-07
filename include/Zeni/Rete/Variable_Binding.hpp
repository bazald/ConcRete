#ifndef ZENI_RETE_VARIABLE_BINDING_HPP
#define ZENI_RETE_VARIABLE_BINDING_HPP

#include "Token_Index.hpp"

#include <unordered_set>

namespace Zeni::Rete {

  typedef std::pair<Token_Index, Token_Index> Variable_Binding;
  typedef std::unordered_set<Variable_Binding> Variable_Bindings;

}

ZENI_RETE_LINKAGE std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Variable_Binding &binding);
ZENI_RETE_LINKAGE std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Variable_Bindings &bindings);

namespace std {
  template <> struct hash<Zeni::Rete::Variable_Binding> {
    size_t operator()(const Zeni::Rete::Variable_Binding &variable_binding) const {
      return Zeni::hash_combine(std::hash<Zeni::Rete::Token_Index>()(variable_binding.first), std::hash<Zeni::Rete::Token_Index>()(variable_binding.second));
    }
  };

  template <> struct hash<Zeni::Rete::Variable_Bindings> {
    size_t operator()(const Zeni::Rete::Variable_Bindings &variable_bindings) const {
      return Zeni::hash_container<Zeni::Rete::Variable_Binding>()(variable_bindings);
    }
  };
}

#endif
