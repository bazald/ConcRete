#include "Zeni/Rete/Internal/Variable_Indices_Impl.hpp"

#include "Zeni/Rete/Node.hpp"

#include <string_view>

namespace Zeni::Rete {

  std::shared_ptr<Variable_Indices_Impl> Variable_Indices_Impl::Create() {
    class Friendly_Variable_Indices_Impl : public Variable_Indices_Impl
    {
    };

    return std::make_shared<Friendly_Variable_Indices_Impl>();
  }

  const std::unordered_multimap<std::string, Token_Index> & Variable_Indices_Impl::get_indices() const {
    return name_to_index;
  }

  Token_Index Variable_Indices_Impl::find_index(const std::string &name) const {
    const auto found = name_to_index.find(name);
    if (found != name_to_index.cend())
      return found->second;
    return Token_Index();
  }

  std::string_view Variable_Indices_Impl::find_name(const Token_Index &index) const {
    const auto found = index_to_name.find(index);
    if (found != index_to_name.cend())
      return found->second;
    return std::string();
  }

  std::string_view Variable_Indices_Impl::find_name_rete(const int64_t rete_row, const int8_t column) const {
    const auto found = rrc_to_both.find(std::make_pair(rete_row, column));
    if (found != rrc_to_both.cend())
      return found->second.first;
    return std::string();
  }

  std::string_view Variable_Indices_Impl::find_name_token(const int64_t token_row, const int8_t column) const {
    const auto found = trc_to_both.find(std::make_pair(token_row, column));
    if (found != trc_to_both.cend())
      return found->second.first;
    return std::string();
  }

  std::pair<std::string_view, Token_Index> Variable_Indices_Impl::find_both_rete(const int64_t rete_row, const int8_t column) const {
    const auto found = rrc_to_both.find(std::make_pair(rete_row, column));
    if (found != rrc_to_both.cend())
      return found->second;
    return std::pair<std::string, Token_Index>();
  }

  std::pair<std::string_view, Token_Index> Variable_Indices_Impl::find_both_token(const int64_t token_row, const int8_t column) const {
    const auto found = trc_to_both.find(std::make_pair(token_row, column));
    if (found != trc_to_both.cend())
      return found->second;
    return std::pair<std::string, Token_Index>();
  }

  void Variable_Indices_Impl::insert(const std::string_view name, const Token_Index &index) {
    index_to_name.emplace(index, name);
    name_to_index.emplace(name, index);

    rrc_to_both.emplace(std::make_pair(index.rete_row, index.column), std::make_pair(name, index));
    trc_to_both.emplace(std::make_pair(index.token_row, index.column), std::make_pair(name, index));
  }

  std::shared_ptr<Variable_Indices> Variable_Indices_Impl::reindex_for_right_parent_node(const Variable_Bindings &bindings, const Node &left, const Node &right) const {
    const int64_t left_size = left.get_size();
    const int64_t left_token_size = left.get_token_size();
    const int64_t right_size = right.get_size();

    auto closure = Variable_Indices::Create();

    for (const auto &index : name_to_index) {
      if (index.second.rete_row >= left_size && index.second.rete_row < left_size + right_size)
        closure->insert(index.first, Token_Index(index.second.rete_row - left_size, index.second.token_row - left_token_size, index.second.column));
    }

    for (const auto &binding : bindings) {
      auto found = find_both_token(binding.first.token_row, binding.first.column);
      if (!found.first.empty()) {
        closure->insert(found.first, binding.second);
        continue;
      }

      found = closure->find_both_token(binding.first.token_row, binding.first.column);
      if (!found.first.empty()) {
        closure->insert(found.first, binding.second);
        continue;
      }
    }

    return closure;
  }

  size_t Variable_Indices_Impl::get_hash() const {
    return hash_container<std::pair<std::string, Token_Index>>()(name_to_index);
  }

  bool Variable_Indices_Impl::operator==(const Variable_Indices &rhs) const {
    if (auto variable_indices_impl = dynamic_cast<const Variable_Indices_Impl *>(&rhs))
      return name_to_index == variable_indices_impl->name_to_index;
    return false;
  }

}
