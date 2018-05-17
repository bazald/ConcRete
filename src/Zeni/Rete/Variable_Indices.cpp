#include "Zeni/Rete/Variable_Indices.h"

#include <iostream>
#include <string>

std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Variable_Indices &indices) {
  os << '{';
  for (const auto &index : indices) {
    os << '[' << index.first << ',' << index.second << ']';
  }
  os << '}';
  return os;
}
