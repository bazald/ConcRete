#include "Zeni/Rete/Variable_Binding.hpp"

#include <iostream>

std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Variable_Binding &binding) {
  return os << binding.first << ':' << binding.second;
}

std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Variable_Bindings &bindings) {
  if(bindings.empty())
    return os << "[]";
  os << '[';
  auto bt = bindings.cbegin();
  os << *bt++;
  while(bt != bindings.cend())
    os << ',' << *bt++;
  os << ']';
  return os;
}
