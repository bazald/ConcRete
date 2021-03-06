#ifndef ZENI_RETE_VARIABLE_INDICES_HPP
#define ZENI_RETE_VARIABLE_INDICES_HPP

#include "Variable_Binding.hpp"

#include <memory>
#include <unordered_map>

namespace Zeni::Rete {

  class Node;
  class Token_Index;

  class Variable_Indices : std::enable_shared_from_this<Variable_Indices> {
    Variable_Indices(const Variable_Indices &) = delete;
    Variable_Indices & operator=(const Variable_Indices &) = delete;

  protected:
    Variable_Indices() = default;

  public:
    ZENI_RETE_LINKAGE virtual ~Variable_Indices() {}

    ZENI_RETE_LINKAGE static std::shared_ptr<Variable_Indices> Create();
    ZENI_RETE_LINKAGE static std::shared_ptr<Variable_Indices> Create(const int64_t left_size, const int64_t left_token_size, const Variable_Indices &left, const Variable_Indices &right);

    ZENI_RETE_LINKAGE virtual const std::unordered_multimap<std::string, Token_Index> & get_indices() const = 0;

    ZENI_RETE_LINKAGE virtual Token_Index find_index(const std::string &name) const = 0;

    ZENI_RETE_LINKAGE virtual std::string_view find_name(const Token_Index &index) const = 0;
    ZENI_RETE_LINKAGE virtual std::string_view find_name_rete(const int64_t rete_row, const int64_t column) const = 0;
    ZENI_RETE_LINKAGE virtual std::string_view find_name_token(const int64_t token_row, const int64_t column) const = 0;

    ZENI_RETE_LINKAGE virtual std::pair<std::string_view, Token_Index> find_both_rete(const int64_t rete_row, const int64_t column) const = 0;
    ZENI_RETE_LINKAGE virtual std::pair<std::string_view, Token_Index> find_both_token(const int64_t token_row, const int64_t column) const = 0;

    ZENI_RETE_LINKAGE virtual void insert(const std::string_view name, const Token_Index &index) = 0;

    ZENI_RETE_LINKAGE virtual std::shared_ptr<Variable_Indices> reindex_for_right_parent_node(const Variable_Bindings &bindings, const Node &left, const Node &right) const = 0;

    virtual size_t get_hash() const = 0;

    virtual bool operator==(const Variable_Indices &rhs) const = 0;
  };

}

std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Variable_Indices &indices);

namespace std {
  template <> struct hash<Zeni::Rete::Variable_Indices> {
    size_t operator()(const Zeni::Rete::Variable_Indices &variable_indices) const {
      return variable_indices.get_hash();
    }
  };
}

#endif
