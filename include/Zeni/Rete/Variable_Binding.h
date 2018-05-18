#ifndef ZENI_RETE_VARIABLE_BINDING_H
#define ZENI_RETE_VARIABLE_BINDING_H

#include "Zeni/Rete/Token_Index.h"

#include <set>

namespace Zeni {

  namespace Rete {

    typedef std::pair<Token_Index, Token_Index> Variable_Binding;
    typedef std::set<Variable_Binding> Variable_Bindings;

  }

}

ZENI_RETE_LINKAGE std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Variable_Binding &binding);
ZENI_RETE_LINKAGE std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Variable_Bindings &bindings);

#endif
