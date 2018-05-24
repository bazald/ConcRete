#include "Zeni/Rete/Working_Memory.hpp"

#include <iostream>

std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Working_Memory &working_memory) {
  os << '{' << std::endl;
  for (const auto &wme : working_memory.wmes)
    os << "  " << *wme << std::endl;
  os << '}';
  return os;
}
