#ifndef ZENI_RETE_TOKEN_H
#define ZENI_RETE_TOKEN_H

#include "Zeni/Rete/Token_Index.h"
#include "Zeni/Rete/Variable_Binding.h"
#include "Zeni/Rete/WME.h"

#include <set>

namespace Zeni {

  namespace Rete {

    class Token;

  }

}

ZENI_RETE_EXTERN template class ZENI_RETE_LINKAGE std::weak_ptr<Zeni::Rete::Token>;
ZENI_RETE_EXTERN template class ZENI_RETE_LINKAGE std::enable_shared_from_this<Zeni::Rete::Token>;

namespace Zeni {

  namespace Rete {

    class Node;
    class Token;

    class ZENI_RETE_LINKAGE Token : public std::enable_shared_from_this<Token> {
    public:
      Token();
      Token(const std::shared_ptr<const WME> &wme);
      Token(const std::shared_ptr<const Token> &first, const std::shared_ptr<const Token> &second);

      std::shared_ptr<const Token> shared() const;
      std::shared_ptr<Token> shared();

      const std::shared_ptr<const WME> & get_wme() const;

      int64_t size() const;

      bool operator==(const Token &rhs) const;
      bool operator!=(const Token &rhs) const;

      size_t get_hash() const { return m_hashval; }

      bool eval_bindings(const bool &from_left, const Variable_Bindings &bindings, const Token &rhs, const bool &rhs_from_left) const;
      size_t hash_bindings(const bool &from_left, const Variable_Bindings &bindings) const;

      std::ostream & print(std::ostream &os) const;

      const std::shared_ptr<const Symbol> & operator[](const Token_Index &index) const;

    private:
      std::pair<std::shared_ptr<const Token>, std::shared_ptr<const Token>> m_token;
      int64_t m_size;

      std::shared_ptr<const WME> m_wme;

      size_t m_hashval;
    };

    //std::shared_ptr<const Variable_Indices> bind_Variable_Indices(const Variable_Bindings &bindings, const std::shared_ptr<const Variable_Indices> &indices, const Node &left, const Node &right);
    std::string get_Variable_name(const std::shared_ptr<const Variable_Indices> &indices, const Token_Index &index);

  }

}

ZENI_RETE_LINKAGE std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Token &Token);

namespace std {
  template <> struct hash<Zeni::Rete::Token> {
    size_t operator()(const Zeni::Rete::Token &Token) const {
      return Token.get_hash();
    }
  };
}

#endif
