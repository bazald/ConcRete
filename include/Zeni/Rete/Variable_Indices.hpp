#ifndef ZENI_RETE_VARIABLE_INDICES_H
#define ZENI_RETE_VARIABLE_INDICES_H

#include "Token_Index.hpp"
#include "Variable_Binding.hpp"

#include <memory>
#include <unordered_map>

namespace Zeni {

  namespace Rete {

    class Node;
    class Variable_Indices_Pimpl;

    class Variable_Indices {
    public:
      ZENI_RETE_LINKAGE Variable_Indices();
      ZENI_RETE_LINKAGE ~Variable_Indices();

      ZENI_RETE_LINKAGE Variable_Indices(const Variable_Indices &rhs);
      ZENI_RETE_LINKAGE Variable_Indices(Variable_Indices &&rhs);

      ZENI_RETE_LINKAGE Variable_Indices & operator=(const Variable_Indices &rhs);
      ZENI_RETE_LINKAGE Variable_Indices & operator=(Variable_Indices &&rhs);

      ZENI_RETE_LINKAGE const std::unordered_multimap<std::string, Token_Index> & get_indices() const;

      ZENI_RETE_LINKAGE Token_Index find_index(const std::string &name) const;

      ZENI_RETE_LINKAGE std::string find_name(const Token_Index &index) const;
      ZENI_RETE_LINKAGE std::string find_name_rete(const int64_t &rete_row, const int8_t &column) const;
      ZENI_RETE_LINKAGE std::string find_name_token(const int64_t &token_row, const int8_t &column) const;

      ZENI_RETE_LINKAGE std::pair<std::string, Token_Index> find_both_rete(const int64_t &rete_row, const int8_t &column) const;
      ZENI_RETE_LINKAGE std::pair<std::string, Token_Index> find_both_token(const int64_t &token_row, const int8_t &column) const;

      ZENI_RETE_LINKAGE void insert(const std::string &name, const Token_Index &index);

      ZENI_RETE_LINKAGE Variable_Indices reindex_for_right_parent_node(const Variable_Bindings &bindings, const Node &left, const Node &right) const;

    private:
      Variable_Indices_Pimpl * const m_impl;
    };

  }

}

std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Variable_Indices &indices);

#endif
