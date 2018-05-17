#ifndef ZENI_RETE_TOKEN_INDEX_H
#define ZENI_RETE_TOKEN_INDEX_H

#include "Zeni/Linkage.h"

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

#endif
