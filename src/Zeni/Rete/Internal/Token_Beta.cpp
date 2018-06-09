#include "Zeni/Rete/Internal/Token_Beta.hpp"

#include <cassert>
#include <iostream>
#include <sstream>

namespace Zeni::Rete {

  Token_Beta::Token_Beta(const std::shared_ptr<const Token> first, const std::shared_ptr<const Token> second)
    : Token(first->size() + second->size()),
    m_first(first),
    m_second(second),
    m_hash(hash_combine(std::hash<Token>()(*first), std::hash<Token>()(*second)))
  {
  }

  void Token_Beta::print(std::ostream &os) const {
    assert(m_first);
    assert(m_second);
    m_first->print(os);
    m_second->print(os);
  }

  std::shared_ptr<const Symbol> Token_Beta::operator[](const Token_Index &index) const {
    assert(index.token_row >= 0);
    assert(index.token_row < size());

    const auto first_size = m_first->size();
    if (index.token_row < first_size)
      return (*m_first)[index];
    else
      return (*m_second)[Token_Index(index.rete_row, index.token_row - first_size, index.column)];
  }

  size_t Token_Beta::get_hash() const {
    return m_hash;
  }

  bool Token_Beta::operator==(const Token &rhs) const {
    if (auto token_beta = dynamic_cast<const Token_Beta *>(&rhs))
      return m_first == token_beta->m_first && m_second == token_beta->m_second;
    return false;
  }

}
