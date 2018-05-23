#ifndef ZENI_RETE_TOKEN_BETA_H
#define ZENI_RETE_TOKEN_BETA_H

#include "Zeni/Rete/Token.hpp"

namespace Zeni {

  namespace Rete {

    class Token_Beta : public Token {
    public:
      ZENI_RETE_LINKAGE Token_Beta(const std::shared_ptr<const Token> &first, const std::shared_ptr<const Token> &second);

      ZENI_RETE_LINKAGE void print(std::ostream &os) const override;

      ZENI_RETE_LINKAGE const std::shared_ptr<const Symbol> & operator[](const Token_Index &index) const override;

    private:
      std::shared_ptr<const Token> m_first;
      std::shared_ptr<const Token> m_second;
    };

    typedef std::unordered_multiset<std::shared_ptr<const Token> /*, Zeni::hash_deref<Token>, Zeni::compare_deref_eq*/> Tokens;

  }

}

#endif
