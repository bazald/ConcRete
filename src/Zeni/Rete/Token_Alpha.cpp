#include "Zeni/Rete/Token_Alpha.hpp"

#include <cassert>
#include <iostream>
#include <sstream>

namespace Zeni::Rete {

  Token_Alpha::Token_Alpha(const std::shared_ptr<const WME> wme)
    : Token(1),
    m_wme(wme)
  {
  }

  const std::shared_ptr<const WME> & Token_Alpha::get_wme() const {
    return m_wme;
  }

  void Token_Alpha::print(std::ostream &os) const {
    assert(m_wme);
    os << *m_wme;
  }

  std::shared_ptr<const Symbol> Token_Alpha::operator[](const Token_Index &index) const {
    assert(index.token_row == 0);

    switch (index.column) {
    case 0: return std::get<0>(m_wme->get_symbols());
    case 1: return std::get<1>(m_wme->get_symbols());
    case 2: return std::get<2>(m_wme->get_symbols());
    default: abort();
    }
  }

  size_t Token_Alpha::get_hash() const {
    return m_wme->get_hash();
  }

  bool Token_Alpha::operator==(const Token &rhs) const {
    if (auto token_alpha = dynamic_cast<const Token_Alpha *>(&rhs))
      return *m_wme == *token_alpha->m_wme;
    return false;
  }

}
