#ifndef ZENI_RETE_TOKEN_INDEX_H
#define ZENI_RETE_TOKEN_INDEX_H

#include "Zeni/Linkage.h"
#include "Zeni/Utility.h"

#include <cinttypes>
#include <iosfwd>

namespace Zeni {

  namespace Rete {

    struct ZENI_RETE_LINKAGE Token_Index {
      Token_Index();
      Token_Index(const int64_t &rete_row_, const int64_t &token_row_, const int8_t &column_, const bool &existential_ = false);

      bool operator==(const Token_Index &rhs) const;
      bool operator!=(const Token_Index &rhs) const;
      bool operator<(const Token_Index &rhs) const;
      bool operator<=(const Token_Index &rhs) const;
      bool operator>(const Token_Index &rhs) const;
      bool operator>=(const Token_Index &rhs) const;

      int64_t rete_row;
      int64_t token_row;
      int8_t column;
      bool existential;
    };

  }

}

std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Token_Index &index);

namespace std {
  template <> struct hash<Zeni::Rete::Token_Index> {
    size_t operator()(const Zeni::Rete::Token_Index &token_index) const {
      return Zeni::hash_combine(Zeni::hash_combine(Zeni::hash_combine(token_index.rete_row, token_index.token_row), token_index.column), token_index.existential);
    }
  };
}

#endif
