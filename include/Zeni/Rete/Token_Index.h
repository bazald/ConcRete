#ifndef ZENI_RETE_TOKEN_INDEX_H
#define ZENI_RETE_TOKEN_INDEX_H

#include "Zeni/Linkage.h"

#include <cinttypes>
#include <iosfwd>

namespace Zeni {

  namespace Rete {

    struct ZENI_RETE_LINKAGE Token_Index {
      Token_Index() : rete_row(0), token_row(0), column(0), existential(false) {}

      Token_Index(const int64_t &rete_row_, const int64_t &token_row_, const int8_t &column_, const bool &existential_ = false)
        : rete_row(rete_row_), token_row(token_row_), column(column_), existential(existential_)
      {
      }

      bool operator==(const Token_Index &rhs) const {
        return rete_row == rhs.rete_row && token_row == rhs.token_row && column == rhs.column && existential == rhs.existential;
      }

      bool operator!=(const Token_Index &rhs) const {
        return rete_row != rhs.rete_row || token_row != rhs.token_row || column != rhs.column || existential != rhs.existential;
      }

      bool operator<(const Token_Index &rhs) const {
        return (!existential && rhs.existential) || (existential == rhs.existential &&
          (rete_row < rhs.rete_row || (rete_row == rhs.rete_row &&
          (token_row < rhs.token_row || (token_row == rhs.token_row &&
            (column < rhs.column))))));
      }

      bool operator<=(const Token_Index &rhs) const {
        return (!existential && rhs.existential) || (existential == rhs.existential &&
          (rete_row < rhs.rete_row || (rete_row == rhs.rete_row &&
          (token_row < rhs.token_row || (token_row == rhs.token_row &&
            (column <= rhs.column))))));
      }

      bool operator>(const Token_Index &rhs) const {
        return rhs <= *this;
      }

      bool operator>=(const Token_Index &rhs) const {
        return rhs < *this;
      }

      int64_t rete_row;
      int64_t token_row;
      int8_t column;
      bool existential;
    };

  }

}

std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Token_Index &index);

#endif
