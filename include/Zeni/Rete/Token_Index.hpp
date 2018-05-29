#ifndef ZENI_RETE_TOKEN_INDEX_H
#define ZENI_RETE_TOKEN_INDEX_H

#include "Zeni/Utility.hpp"
#include "Linkage.hpp"

#include <cinttypes>
#include <iosfwd>

namespace Zeni::Rete {

  class ZENI_RETE_LINKAGE Token_Index {
  public:
    Token_Index();
    Token_Index(const int64_t rete_row_, const int64_t token_row_, const int8_t column_);

    bool operator==(const Token_Index &rhs) const;
    bool operator!=(const Token_Index &rhs) const;
    bool operator<(const Token_Index &rhs) const;
    bool operator<=(const Token_Index &rhs) const;
    bool operator>(const Token_Index &rhs) const;
    bool operator>=(const Token_Index &rhs) const;

    int64_t rete_row;
    int64_t token_row;
    int8_t column;
  };

}

std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Token_Index &index);

namespace std {
  template <> struct hash<Zeni::Rete::Token_Index> {
    size_t operator()(const Zeni::Rete::Token_Index &token_index) const {
      return Zeni::hash_combine(Zeni::hash_combine(size_t(token_index.rete_row), size_t(token_index.token_row)), token_index.column);
    }
  };
}

#endif
