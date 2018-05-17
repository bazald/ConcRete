#include "wme.h"

namespace Rete {

  WME::WME(const Symbol_Ptr_C &first, const Symbol_Ptr_C &second, const Symbol_Ptr_C &third) {
    symbols[0] = first;
    symbols[1] = second;
    symbols[2] = third;
  }

  bool WME::operator==(const WME &rhs) const {
    return (symbols[0] ? rhs.symbols[0] && (*symbols[0] == *rhs.symbols[0]) : !rhs.symbols[0]) &&
           (symbols[1] ? rhs.symbols[1] && (*symbols[1] == *rhs.symbols[1]) : !rhs.symbols[1]) &&
           (symbols[2] ? rhs.symbols[2] && (*symbols[2] == *rhs.symbols[2]) : !rhs.symbols[2]);
  }

  bool WME::operator<(const WME &rhs) const {
    return *symbols[0] < *rhs.symbols[0] || (*symbols[0] == *rhs.symbols[0] &&
          (*symbols[1] < *rhs.symbols[1] || (*symbols[1] == *rhs.symbols[1] && *symbols[2] < *rhs.symbols[2])));
  }

  std::ostream & WME::print(std::ostream &os) const {
//    return os << '(' << symbols[0] << ':' << *symbols[0] << " ^" << symbols[1] << ':' << *symbols[1] << ' ' << symbols[2] << ':' << *symbols[2] << ')';
    return os << '(' << *symbols[0] << " ^" << *symbols[1] << ' ' << *symbols[2] << ')';
  }

  std::ostream & WME::print(std::ostream &os, const Variable_Indices_Ptr_C &indices) const {
    os << '(';
    symbols[0]->print(os, indices);
    os << " ^";
    symbols[1]->print(os, indices);
    os << ' ';
    symbols[2]->print(os, indices);
    os << ')';
    return os;
  }

}

std::ostream & operator<<(std::ostream &os, const Rete::WME &wme) {
  return wme.print(os);
}
