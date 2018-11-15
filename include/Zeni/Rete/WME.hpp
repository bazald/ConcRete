#ifndef ZENI_RETE_WME_HPP
#define ZENI_RETE_WME_HPP

#include "Zeni/Utility.hpp"
#include "Symbol.hpp"

namespace Zeni::Rete {

  class WME {
  public:
    typedef std::tuple<std::shared_ptr<const Symbol>, std::shared_ptr<const Symbol>, std::shared_ptr<const Symbol>> Symbols;

    ZENI_RETE_LINKAGE WME();
    ZENI_RETE_LINKAGE WME(const std::shared_ptr<const Symbol> first, const std::shared_ptr<const Symbol> second, const std::shared_ptr<const Symbol> third);

    ZENI_RETE_LINKAGE bool operator==(const WME &rhs) const;
    ZENI_RETE_LINKAGE bool operator<(const WME &rhs) const;

    ZENI_RETE_LINKAGE const Symbols & get_symbols() const;
    ZENI_RETE_LINKAGE size_t get_hash() const;

    ZENI_RETE_LINKAGE std::ostream & print(std::ostream &os) const;

  private:
    Symbols m_symbols;

    size_t m_hashval;
  };

}

ZENI_RETE_LINKAGE std::ostream & operator<<(std::ostream &os, const Zeni::Rete::WME &wme);

namespace std {
  template <> struct hash<Zeni::Rete::WME> {
    size_t operator()(const Zeni::Rete::WME &wme) const {
      return wme.get_hash();
    }
  };
}

#endif
