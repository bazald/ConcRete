#ifndef ZENI_RETE_TOKEN_HPP
#define ZENI_RETE_TOKEN_HPP

#include "Token_Index.hpp"
#include "Variable_Binding.hpp"
#include "WME.hpp"

#include <unordered_set>

namespace Zeni::Rete {

  class Node;

  class Token : public std::enable_shared_from_this<Token> {
  public:
    ZENI_RETE_LINKAGE Token(const int64_t size);

    ZENI_RETE_LINKAGE int64_t size() const;

    ZENI_RETE_LINKAGE bool eval_bindings(const bool from_left, const Variable_Bindings &bindings, const Token &rhs, const bool rhs_from_left) const;
    ZENI_RETE_LINKAGE size_t hash_bindings(const bool from_left, const Variable_Bindings &bindings) const;

    ZENI_RETE_LINKAGE virtual void print(std::ostream &os) const = 0;

    ZENI_RETE_LINKAGE virtual std::shared_ptr<const Symbol> operator[](const Token_Index &index) const = 0;

  private:
    int64_t m_size;
  };

  typedef std::unordered_multiset<std::shared_ptr<const Token> /*, Zeni::hash_deref<Token>, Zeni::compare_deref_eq*/> Token_Multiset;

  struct Tokens_Input {
    Token_Multiset positive;
    Token_Multiset negative;
  };

  typedef Token_Multiset Tokens_Output;

}

ZENI_RETE_LINKAGE std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Token &token);

#endif
