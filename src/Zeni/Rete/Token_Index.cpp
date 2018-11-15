#include "Zeni/Rete/Token_Index.hpp"

#include <iostream>

namespace Zeni::Rete {

  Token_Index::Token_Index()
    : rete_row(-1), token_row(-1), column(-1)
  {
  }

  Token_Index::Token_Index(const int64_t rete_row_, const int64_t token_row_, const int64_t column_)
    : rete_row(rete_row_), token_row(token_row_), column(column_)
  {
  }

  bool Token_Index::operator==(const Token_Index &rhs) const {
    return rete_row == rhs.rete_row && token_row == rhs.token_row && column == rhs.column;
  }

  bool Token_Index::operator!=(const Token_Index &rhs) const {
    return rete_row != rhs.rete_row || token_row != rhs.token_row || column != rhs.column;
  }

  bool Token_Index::operator<(const Token_Index &rhs) const {
    return (rete_row < rhs.rete_row || (rete_row == rhs.rete_row &&
      (token_row < rhs.token_row || (token_row == rhs.token_row &&
      (column < rhs.column)))));
  }

  bool Token_Index::operator<=(const Token_Index &rhs) const {
    return (rete_row < rhs.rete_row || (rete_row == rhs.rete_row &&
      (token_row < rhs.token_row || (token_row == rhs.token_row &&
      (column <= rhs.column)))));
  }

  bool Token_Index::operator>(const Token_Index &rhs) const {
    return rhs <= *this;
  }

  bool Token_Index::operator>=(const Token_Index &rhs) const {
    return rhs < *this;
  }

}

std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Token_Index &index) {
  return os << '(' << index.rete_row << ',' << index.token_row << ',' << int(index.column) << ')';
}
