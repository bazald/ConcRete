#ifndef WME_H
#define WME_H

#include "../utility/memory_pool.h"

#include "symbol.h"
#include "utility.h"

namespace Rete {

  class WME;
  typedef std::shared_ptr<const WME> WME_Ptr_C;
  typedef std::shared_ptr<WME> WME_Ptr;

  class RETE_LINKAGE WME : public Zeni::Pool_Allocator<WME> {
  public:
    typedef std::array<Symbol_Ptr_C, 3> WME_Symbols;

    WME() {}
    WME(const Symbol_Ptr_C &first, const Symbol_Ptr_C &second, const Symbol_Ptr_C &third);

    bool operator==(const WME &rhs) const;
    bool operator<(const WME &rhs) const;

    std::ostream & print(std::ostream &os) const;
    std::ostream & print(std::ostream &os, const Variable_Indices_Ptr_C &indices) const;

    WME_Symbols symbols;
  };

}

RETE_LINKAGE std::ostream & operator<<(std::ostream &os, const Rete::WME &wme);

namespace std {
  template <> struct hash<Rete::WME> {
    size_t operator()(const Rete::WME &wme) const {
      return Rete::hash_combine(Rete::hash_combine(wme.symbols[0] ? wme.symbols[0]->hash() : 0,
                                                   wme.symbols[1] ? wme.symbols[1]->hash() : 0),
                                wme.symbols[2] ? wme.symbols[2]->hash() : 0);
    }
  };
}

#endif
