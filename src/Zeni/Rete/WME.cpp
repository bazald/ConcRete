#include "Zeni/Rete/WME.hpp"

#include <iostream>

namespace Zeni {

  namespace Rete {

    WME::WME()
      : m_hashval(0)
    {
    }

    WME::WME(const std::shared_ptr<const Symbol> &first, const std::shared_ptr<const Symbol> &second, const std::shared_ptr<const Symbol> &third)
      : m_symbols(first, second, third),
      m_hashval(Zeni::hash_combine(Zeni::hash_combine(first ? first->hash() : 0,
        second ? second->hash() : 0),
        third ? third->hash() : 0))
    {
    }

    bool WME::operator==(const WME &rhs) const {
      return (std::get<0>(m_symbols) ? std::get<0>(rhs.m_symbols) && (*std::get<0>(m_symbols) == *std::get<0>(rhs.m_symbols)) : !std::get<0>(rhs.m_symbols)) &&
        (std::get<1>(m_symbols) ? std::get<1>(rhs.m_symbols) && (*std::get<1>(m_symbols) == *std::get<1>(rhs.m_symbols)) : !std::get<1>(rhs.m_symbols)) &&
        (std::get<2>(m_symbols) ? std::get<2>(rhs.m_symbols) && (*std::get<2>(m_symbols) == *std::get<2>(rhs.m_symbols)) : !std::get<2>(rhs.m_symbols));
    }

    bool WME::operator<(const WME &rhs) const {
      return *std::get<0>(m_symbols) < *std::get<0>(rhs.m_symbols) || (*std::get<0>(m_symbols) == *std::get<0>(rhs.m_symbols) &&
        (*std::get<1>(m_symbols) < *std::get<1>(rhs.m_symbols) || (*std::get<1>(m_symbols) == *std::get<1>(rhs.m_symbols) && *std::get<2>(m_symbols) < *std::get<2>(rhs.m_symbols))));
    }

    const WME::Symbols & WME::get_symbols() const {
      return m_symbols;
    }

    size_t WME::get_hash() const {
      return m_hashval;
    }

    std::ostream & WME::print(std::ostream &os) const {
      //    return os << '(' << std::get<0>(m_symbols) << ':' << *std::get<0>(m_symbols) << " ^" << std::get<1>(m_symbols) << ':' << *std::get<1>(m_symbols) << ' ' << std::get<2>(m_symbols) << ':' << *std::get<2>(m_symbols) << ')';
      return os << '(' << *std::get<0>(m_symbols) << " ^" << *std::get<1>(m_symbols) << ' ' << *std::get<2>(m_symbols) << ')';
    }

    std::ostream & WME::print(std::ostream &os, const std::shared_ptr<const Variable_Indices> &indices) const {
      os << '(';
      std::get<0>(m_symbols)->print(os, indices);
      os << " ^";
      std::get<1>(m_symbols)->print(os, indices);
      os << ' ';
      std::get<2>(m_symbols)->print(os, indices);
      os << ')';
      return os;
    }

  }

}

std::ostream & operator<<(std::ostream &os, const Zeni::Rete::WME &wme) {
  return wme.print(os);
}
