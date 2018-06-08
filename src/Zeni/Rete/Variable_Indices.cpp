#include "Zeni/Rete/Internal/Variable_Indices_Impl.hpp"

namespace Zeni::Rete {

  std::shared_ptr<Variable_Indices> Variable_Indices::Create() {
    return Variable_Indices_Impl::Create();
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
