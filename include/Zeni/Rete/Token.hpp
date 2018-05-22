#ifndef ZENI_RETE_TOKEN_H
#define ZENI_RETE_TOKEN_H

#include "Zeni/Rete/Token_Index.hpp"
#include "Zeni/Rete/Variable_Binding.hpp"
#include "Zeni/Rete/WME.hpp"

#include <set>

namespace Zeni {

  namespace Rete {

    class Token;

  }

}

namespace Zeni {

  namespace Rete {

    class Node;
    class Token;

    class Token : public std::enable_shared_from_this<Token> {
    public:
      ZENI_RETE_LINKAGE Token();
      ZENI_RETE_LINKAGE Token(const std::shared_ptr<const WME> &wme);
      ZENI_RETE_LINKAGE Token(const std::shared_ptr<const Token> &first, const std::shared_ptr<const Token> &second);

      ZENI_RETE_LINKAGE std::shared_ptr<const Token> shared() const;
      ZENI_RETE_LINKAGE std::shared_ptr<Token> shared();

      ZENI_RETE_LINKAGE const std::shared_ptr<const WME> & get_wme() const;

      ZENI_RETE_LINKAGE int64_t size() const;

      ZENI_RETE_LINKAGE bool operator==(const Token &rhs) const;
      ZENI_RETE_LINKAGE bool operator!=(const Token &rhs) const;

      ZENI_RETE_LINKAGE size_t get_hash() const { return m_hashval; }

      ZENI_RETE_LINKAGE bool eval_bindings(const bool &from_left, const Variable_Bindings &bindings, const Token &rhs, const bool &rhs_from_left) const;
      ZENI_RETE_LINKAGE size_t hash_bindings(const bool &from_left, const Variable_Bindings &bindings) const;

      ZENI_RETE_LINKAGE std::ostream & print(std::ostream &os) const;

      ZENI_RETE_LINKAGE const std::shared_ptr<const Symbol> & operator[](const Token_Index &index) const;

    private:
      std::shared_ptr<const Token> m_first;
      std::shared_ptr<const Token> m_second;
      int64_t m_size;

      std::shared_ptr<const WME> m_wme;

      size_t m_hashval;
    };

    std::string get_Variable_name(const std::shared_ptr<const Variable_Indices> &indices, const Token_Index &index);

  }

}

ZENI_RETE_LINKAGE std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Token &token);

namespace std {
  template <> struct hash<Zeni::Rete::Token> {
    size_t operator()(const Zeni::Rete::Token &token) const {
      return token.get_hash();
    }
  };
}

#endif
