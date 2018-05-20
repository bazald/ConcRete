#include "Zeni/Rete/WME.hpp"

namespace Zeni {

  namespace Rete {

    WME::WME(const std::shared_ptr<const Symbol> &first, const std::shared_ptr<const Symbol> &second, const std::shared_ptr<const Symbol> &third) {
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

    std::ostream & WME::print(std::ostream &os, const std::shared_ptr<const Variable_Indices> &indices) const {
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

}

std::ostream & operator<<(std::ostream &os, const Zeni::Rete::WME &wme) {
  return wme.print(os);
}
