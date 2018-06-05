#ifndef ZENI_RETE_TOKEN_BETA_HPP
#define ZENI_RETE_TOKEN_BETA_HPP

#include "Token.hpp"

namespace Zeni::Rete {

  class Token_Beta : public Token {
  public:
    ZENI_RETE_LINKAGE Token_Beta(const std::shared_ptr<const Token> first, const std::shared_ptr<const Token> second);

    ZENI_RETE_LINKAGE void print(std::ostream &os) const override;

    ZENI_RETE_LINKAGE std::shared_ptr<const Symbol> operator[](const Token_Index &index) const override;

  private:
    std::shared_ptr<const Token> m_first;
    std::shared_ptr<const Token> m_second;
  };

}

#endif
