#ifndef WME_SET_H
#define WME_SET_H

#include "wme.h"
#include "utility.h"

namespace Rete {

  class RETE_LINKAGE WME_Set {
  public:
    std::unordered_set<WME_Ptr_C, Rete::hash_deref<Rete::WME>, Rete::compare_deref_eq> wmes;
  };

}

inline std::ostream & operator<<(std::ostream &os, const Rete::WME_Set &wme_set) {
  os << '{' << std::endl;
  for(const auto &wme : wme_set.wmes)
    os << "  " << *wme << std::endl;
  os << '}';
  return os;
}

namespace std {
  template <> struct hash<Rete::WME_Set> {
    size_t operator()(const Rete::WME_Set &wme_set) const {
      return hash<std::unordered_set<Rete::WME_Ptr_C, Rete::hash_deref<Rete::WME>, Rete::compare_deref_eq>>()(wme_set.wmes);
    }
  };
}

#endif
