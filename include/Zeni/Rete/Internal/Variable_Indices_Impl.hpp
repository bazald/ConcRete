#ifndef ZENI_RETE_VARIABLE_INDICES_IMPL_HPP
#define ZENI_RETE_VARIABLE_INDICES_IMPL_HPP

#include "../Variable_Indices.hpp"

namespace Zeni::Rete {

  class Variable_Indices_Impl : public Variable_Indices {
    Variable_Indices_Impl(const Variable_Indices_Impl &) = delete;
    Variable_Indices_Impl & operator=(const Variable_Indices_Impl &) = delete;

    Variable_Indices_Impl() = default;

  public:
    static std::shared_ptr<Variable_Indices_Impl> Create();
    static std::shared_ptr<Variable_Indices_Impl> Create(const int64_t left_size, const int64_t left_token_size, const Variable_Indices &left, const Variable_Indices &right);

    const std::unordered_multimap<std::string, Token_Index> & get_indices() const override;

    Token_Index find_index(const std::string &name) const override;

    std::string_view find_name(const Token_Index &index) const override;
    std::string_view find_name_rete(const int64_t rete_row, const int8_t column) const override;
    std::string_view find_name_token(const int64_t token_row, const int8_t column) const override;

    std::pair<std::string_view, Token_Index> find_both_rete(const int64_t rete_row, const int8_t column) const override;
    std::pair<std::string_view, Token_Index> find_both_token(const int64_t token_row, const int8_t column) const override;

    void insert(const std::string_view name, const Token_Index &index) override;

    std::shared_ptr<Variable_Indices> reindex_for_right_parent_node(const Variable_Bindings &bindings, const Node &left, const Node &right) const override;

    size_t get_hash() const override;

    bool operator==(const Variable_Indices &rhs) const override;

  private:
    std::unordered_multimap<Token_Index, std::string> index_to_name;
    std::unordered_multimap<std::string, Token_Index> name_to_index;

    std::unordered_multimap<std::pair<int64_t, int64_t>, std::pair<std::string, Token_Index>> rrc_to_both;
    std::unordered_multimap<std::pair<int64_t, int64_t>, std::pair<std::string, Token_Index>> trc_to_both;
  };

}

#endif
