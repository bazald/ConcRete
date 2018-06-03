#ifndef ZENI_RETE_VARIABLE_INDICES_HPP
#define ZENI_RETE_VARIABLE_INDICES_HPP

#include "Variable_Binding.hpp"

#include <memory>
#include <unordered_map>

namespace Zeni::Rete {

  class Node;
  class Token_Index;
  class Variable_Indices_Pimpl;

  class Variable_Indices : std::enable_shared_from_this<Variable_Indices> {
#if defined(_WIN32) && defined(NDEBUG)
    static const int m_pimpl_size = 256;
#elif defined(_WIN32)
    static const int m_pimpl_size = 320;
#else
    static const int m_pimpl_size = 224;
#endif
    static const int m_pimpl_align = 8;
    const Variable_Indices_Pimpl * get_pimpl() const;
    Variable_Indices_Pimpl * get_pimpl();

  public:
    ZENI_RETE_LINKAGE Variable_Indices();
    ZENI_RETE_LINKAGE ~Variable_Indices();

    ZENI_RETE_LINKAGE Variable_Indices(const Variable_Indices &rhs);
    ZENI_RETE_LINKAGE Variable_Indices(Variable_Indices &&rhs);

    ZENI_RETE_LINKAGE Variable_Indices & operator=(const Variable_Indices &rhs);
    ZENI_RETE_LINKAGE Variable_Indices & operator=(Variable_Indices &&rhs);

    ZENI_RETE_LINKAGE const std::unordered_multimap<std::string, Token_Index> & get_indices() const;

    ZENI_RETE_LINKAGE Token_Index find_index(const std::string &name) const;

    ZENI_RETE_LINKAGE std::string_view find_name(const Token_Index &index) const;
    ZENI_RETE_LINKAGE std::string_view find_name_rete(const int64_t rete_row, const int8_t column) const;
    ZENI_RETE_LINKAGE std::string_view find_name_token(const int64_t token_row, const int8_t column) const;

    ZENI_RETE_LINKAGE std::pair<std::string_view, Token_Index> find_both_rete(const int64_t rete_row, const int8_t column) const;
    ZENI_RETE_LINKAGE std::pair<std::string_view, Token_Index> find_both_token(const int64_t token_row, const int8_t column) const;

    ZENI_RETE_LINKAGE void insert(const std::string_view name, const Token_Index &index);

    ZENI_RETE_LINKAGE Variable_Indices reindex_for_right_parent_node(const Variable_Bindings &bindings, const Node &left, const Node &right) const;

  private:
    alignas(m_pimpl_align) char m_pimpl_storage[m_pimpl_size];
  };

}

std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Variable_Indices &indices);

#endif
