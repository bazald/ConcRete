#ifndef ZENI_RETE_VARIABLE_INDICES_H
#define ZENI_RETE_VARIABLE_INDICES_H

#include "Token_Index.h"
#include "Variable_Binding.h"

#include <unordered_map>
#include <memory>

namespace Zeni {

  namespace Rete {

    class Node;

    typedef std::unordered_multimap<std::string, Token_Index> Variable_Indices;

    std::shared_ptr<const Variable_Indices> bind_Variable_Indices(const Variable_Bindings &bindings, const std::shared_ptr<const Variable_Indices> &indices, const Node &left, const Node &right);

  }

}

std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Variable_Indices &indices);

#endif
