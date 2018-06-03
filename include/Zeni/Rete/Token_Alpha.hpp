#ifndef ZENI_RETE_TOKEN_ALPHA_HPP
#define ZENI_RETE_TOKEN_ALPHA_HPP

#include "Token.hpp"

namespace Zeni::Rete {

  class Token_Alpha : public Token {
  public:
    ZENI_RETE_LINKAGE Token_Alpha(const std::shared_ptr<const WME> wme);

    ZENI_RETE_LINKAGE const std::shared_ptr<const WME> & get_wme() const;

    ZENI_RETE_LINKAGE void print(std::ostream &os) const override;

    ZENI_RETE_LINKAGE std::shared_ptr<const Symbol> operator[](const Token_Index &index) const override;

  private:
    std::shared_ptr<const WME> m_wme;
  };

  typedef std::unordered_multiset<std::shared_ptr<const Token> /*, Zeni::hash_deref<Token>, Zeni::compare_deref_eq*/> Tokens;

}

#endif
