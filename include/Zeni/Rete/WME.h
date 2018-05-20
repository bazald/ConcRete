#ifndef ZENI_RETE_WME_H
#define ZENI_RETE_WME_H

#include "Zeni/Utility.h"
#include "Symbol.h"

#include <array>

namespace Zeni {

  namespace Rete {

    class WME;

    class ZENI_RETE_LINKAGE WME {
    public:
      typedef std::array<std::shared_ptr<const Symbol>, 3> WME_Symbols;

      WME() {}
      WME(const std::shared_ptr<const Symbol> &first, const std::shared_ptr<const Symbol> &second, const std::shared_ptr<const Symbol> &third);

      bool operator==(const WME &rhs) const;
      bool operator<(const WME &rhs) const;

      std::ostream & print(std::ostream &os) const;
      std::ostream & print(std::ostream &os, const std::shared_ptr<const Variable_Indices> &indices) const;

      WME_Symbols symbols;
    };

  }

}

ZENI_RETE_LINKAGE std::ostream & operator<<(std::ostream &os, const Zeni::Rete::WME &wme);

namespace std {
  template <> struct hash<Zeni::Rete::WME> {
    size_t operator()(const Zeni::Rete::WME &wme) const {
      return Zeni::hash_combine(Zeni::hash_combine(wme.symbols[0] ? wme.symbols[0]->hash() : 0,
                                                   wme.symbols[1] ? wme.symbols[1]->hash() : 0),
                                wme.symbols[2] ? wme.symbols[2]->hash() : 0);
    }
  };
}

#endif
