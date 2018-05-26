#include "Zeni/Rete/Token.hpp"

#include <cassert>
#include <iostream>
#include <sstream>

namespace Zeni::Rete {

  Token::Token(const int64_t size)
    : m_size(size)
  {
  }

  int64_t Token::size() const {
    return m_size;
  }

  bool Token::eval_bindings(const bool from_left, const Variable_Bindings &bindings, const Token &rhs, const bool rhs_from_left) const {
    for (auto &binding : bindings) {
      if (*(*this)[from_left ? binding.first : binding.second] != *(rhs)[rhs_from_left ? binding.first : binding.second])
        return false;
    }
    return true;
  }

  size_t Token::hash_bindings(const bool from_left, const Variable_Bindings &bindings) const {
    size_t hashval = 0;
    for (const auto &binding : bindings)
      hashval = hash_combine(hashval, (*this)[from_left ? binding.first : binding.second]->hash());
    return hashval;
  }

}

std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Token &token) {
  os << '{';
  token.print(os);
  os << '}';
  return os;
}
