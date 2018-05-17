#include <Zeni/Rete/Token_Index.h>

#include <iostream>

std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Token_Index &index) {
  return os << '(' << index.rete_row << ',' << index.token_row << ',' << int(index.column) << ',' << index.existential << ')';
}
