#include "Zeni/Rete/Variable_Indices.hpp"

#include "Zeni/Rete/Node.hpp"

#include <iostream>
#include <string_view>

namespace Zeni::Rete {

  class Variable_Indices_Pimpl {
  public:
    Variable_Indices_Pimpl() {}

    Variable_Indices_Pimpl(const Variable_Indices_Pimpl &rhs)
      : index_to_name(rhs.index_to_name),
      name_to_index(rhs.name_to_index),
      rrc_to_both(rhs.rrc_to_both),
      trc_to_both(rhs.trc_to_both)
    {
    }

    Variable_Indices_Pimpl(Variable_Indices_Pimpl &&rhs)
      : index_to_name(std::move(rhs.index_to_name)),
      name_to_index(std::move(rhs.name_to_index)),
      rrc_to_both(std::move(rhs.rrc_to_both)),
      trc_to_both(std::move(rhs.trc_to_both))
    {
    }

    Variable_Indices_Pimpl & operator=(const Variable_Indices_Pimpl &rhs) {
      index_to_name = rhs.index_to_name;
      name_to_index = rhs.name_to_index;
      rrc_to_both = rhs.rrc_to_both;
      trc_to_both = rhs.trc_to_both;
      return *this;
    }

    Variable_Indices_Pimpl & operator=(Variable_Indices_Pimpl &&rhs) {
      index_to_name = std::move(rhs.index_to_name);
      name_to_index = std::move(rhs.name_to_index);
      rrc_to_both = std::move(rhs.rrc_to_both);
      trc_to_both = std::move(rhs.trc_to_both);
      return *this;
    }

    const std::unordered_multimap<std::string_view, Token_Index> & get_indices() const {
      return name_to_index;
    }

    Token_Index find_index(const std::string_view name) const {
      const auto found = name_to_index.find(name);
      if (found != name_to_index.end())
        return found->second;
      return Token_Index();
    }

    std::string_view find_name(const Token_Index &index) const {
      const auto found = index_to_name.find(index);
      if (found != index_to_name.end())
        return found->second;
      return std::string();
    }

    std::string find_name_rete(const int64_t rete_row, const int8_t column) const {
      const auto found = rrc_to_both.find(std::make_pair(rete_row, column));
      if (found != rrc_to_both.end())
        return found->second.first;
      return std::string();
    }

    std::string find_name_token(const int64_t token_row, const int8_t column) const {
      const auto found = trc_to_both.find(std::make_pair(token_row, column));
      if (found != trc_to_both.end())
        return found->second.first;
      return std::string();
    }

    std::pair<std::string, Token_Index> find_both_rete(const int64_t rete_row, const int8_t column) const {
      const auto found = rrc_to_both.find(std::make_pair(rete_row, column));
      if (found != rrc_to_both.end())
        return found->second;
      return std::pair<std::string, Token_Index>();
    }

    std::pair<std::string, Token_Index> find_both_token(const int64_t token_row, const int8_t column) const {
      const auto found = trc_to_both.find(std::make_pair(token_row, column));
      if (found != trc_to_both.end())
        return found->second;
      return std::pair<std::string, Token_Index>();
    }

    void insert(const std::string_view name, const Token_Index &index) {
      index_to_name.insert(std::make_pair(index, name));
      name_to_index.insert(std::make_pair(name, index));

      rrc_to_both.insert(std::make_pair(std::make_pair(index.rete_row, index.column), std::make_pair(name, index)));
      trc_to_both.insert(std::make_pair(std::make_pair(index.token_row, index.column), std::make_pair(name, index)));
    }

    Variable_Indices reindex_for_right_parent_node(const Variable_Bindings &bindings, const Node &left, const Node &right) const {
      const int64_t left_size = left.get_size();
      const int64_t left_token_size = left.get_token_size();
      const int64_t right_size = right.get_size();

      Variable_Indices closure;

      for (const auto &index : name_to_index) {
        if (index.second.rete_row >= left_size && index.second.rete_row < left_size + right_size)
          closure.insert(index.first, Token_Index(index.second.rete_row - left_size, index.second.token_row - left_token_size, index.second.column));
      }

      for (const auto &binding : bindings) {
        auto found = find_both_token(binding.first.token_row, binding.first.column);
        if (!found.first.empty()) {
          closure.insert(found.first, binding.second);
          continue;
        }

        found = closure.find_both_token(binding.first.token_row, binding.first.column);
        if (!found.first.empty()) {
          closure.insert(found.first, binding.second);
          continue;
        }
      }

      return closure;
    }

  private:
    std::unordered_multimap<Token_Index, std::string_view> index_to_name;
    std::unordered_multimap<std::string_view, Token_Index> name_to_index;

    std::unordered_multimap<std::pair<int64_t, int64_t>, std::pair<std::string, Token_Index>> rrc_to_both;
    std::unordered_multimap<std::pair<int64_t, int64_t>, std::pair<std::string, Token_Index>> trc_to_both;
  };

  Variable_Indices::Variable_Indices()
    : m_impl(new Variable_Indices_Pimpl)
  {
  }

  Variable_Indices::~Variable_Indices() {
    delete m_impl;
  }

  Variable_Indices::Variable_Indices(const Variable_Indices &rhs)
    : m_impl(new Variable_Indices_Pimpl(*rhs.m_impl))
  {
  }

  Variable_Indices::Variable_Indices(Variable_Indices &&rhs)
    : m_impl(std::move(rhs.m_impl))
  {
  }

  Variable_Indices & Variable_Indices::operator=(const Variable_Indices &rhs) {
    *m_impl = *rhs.m_impl;
    return *this;
  }

  Variable_Indices & Variable_Indices::operator=(Variable_Indices &&rhs) {
    *m_impl = std::move(*rhs.m_impl);
    return *this;
  }

  const std::unordered_multimap<std::string_view, Token_Index> & Variable_Indices::get_indices() const {
    return m_impl->get_indices();
  }

  Token_Index Variable_Indices::find_index(const std::string_view name) const {
    return m_impl->find_index(name);
  }

  std::string_view Variable_Indices::find_name(const Token_Index &index) const {
    return m_impl->find_name(index);
  }

  std::string_view Variable_Indices::find_name_rete(const int64_t rete_row, const int8_t column) const {
    return m_impl->find_name_rete(rete_row, column);
  }

  std::string_view Variable_Indices::find_name_token(const int64_t token_row, const int8_t column) const {
    return m_impl->find_name_token(token_row, column);
  }

  std::pair<std::string_view, Token_Index> Variable_Indices::find_both_rete(const int64_t rete_row, const int8_t column) const {
    return m_impl->find_both_rete(rete_row, column);
  }

  std::pair<std::string_view, Token_Index> Variable_Indices::find_both_token(const int64_t token_row, const int8_t column) const {
    return m_impl->find_both_token(token_row, column);
  }

  void Variable_Indices::insert(const std::string_view name, const Token_Index &index) {
    m_impl->insert(name, index);
  }

  Variable_Indices Variable_Indices::reindex_for_right_parent_node(const Variable_Bindings &bindings, const Node &left, const Node &right) const {
    return m_impl->reindex_for_right_parent_node(bindings, left, right);
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
