#include "Zeni/Rete/Variable_Indices.hpp"

#include "Zeni/Rete/Node.hpp"

#include <iostream>
#include <string>

namespace Zeni {

  namespace Rete {

    std::shared_ptr<const Variable_Indices> bind_Variable_Indices(const Variable_Bindings &bindings, const std::shared_ptr<const Variable_Indices> &indices, const Node &left, const Node &right) {
      const int64_t left_size = left.get_size();
      const int64_t left_token_size = left.get_token_size();
      const int64_t right_size = right.get_size();

      std::shared_ptr<Variable_Indices> closure = std::make_shared<Variable_Indices>();

      for (const auto &index : *indices) {
        if (index.second.rete_row >= left_size && index.second.rete_row < left_size + right_size)
          closure->insert(std::make_pair(index.first, Token_Index(index.second.rete_row - left_size, index.second.token_row - left_token_size, index.second.column)));
      }

      for (const auto &binding : bindings) {
        auto found = std::find_if(indices->begin(), indices->end(), [binding](const std::pair<std::string, Token_Index> &ind)->bool {
          return ind.second.token_row == binding.first.token_row && ind.second.column == binding.first.column;
        });
        if (found != indices->end()) {
          closure->insert(std::make_pair(found->first, binding.second));
          continue;
        }

        found = std::find_if(closure->begin(), closure->end(), [binding](const std::pair<std::string, Token_Index> &ind)->bool {
          return ind.second.token_row == binding.first.token_row && ind.second.column == binding.first.column;
        });
        if (found != closure->end())
          closure->insert(std::make_pair(found->first, binding.second));
      }

      return closure;
    }

  }

}

std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Variable_Indices &indices) {
  os << '{';
  for (const auto &index : indices) {
    os << '[' << index.first << ',' << index.second << ']';
  }
  os << '}';
  return os;
}
