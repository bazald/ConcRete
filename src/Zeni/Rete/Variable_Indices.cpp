#include "Zeni/Rete/Internal/Variable_Indices_Impl.hpp"

#include <iostream>

namespace Zeni::Rete {

  std::shared_ptr<Variable_Indices> Variable_Indices::Create() {
    return Variable_Indices_Impl::Create();
  }

  std::shared_ptr<Variable_Indices> Variable_Indices::Create(const int64_t left_size, const int64_t left_token_size, const Variable_Indices &left, const Variable_Indices &right) {
    return Variable_Indices_Impl::Create(left_size, left_token_size, left, right);
  }

}

std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Variable_Indices &indices) {
  os << '{';
  for (const auto &index : indices.get_indices()) {
    os << '[' << index.first << ',' << index.second << ']';
  }
  os << '}';
  return os;
}
