#ifndef ZENI_RETE_WORKING_MEMORY_H
#define ZENI_RETE_WORKING_MEMORY_H

#include "Zeni/Utility.hpp"
#include "WME.hpp"

namespace Zeni {

  namespace Rete {

    class Working_Memory {
    public:
      ZENI_RETE_LINKAGE const std::unordered_set<std::shared_ptr<const WME>, hash_deref<WME>, compare_deref_eq> & get_wmes() const;
      ZENI_RETE_LINKAGE std::unordered_set<std::shared_ptr<const WME>, hash_deref<WME>, compare_deref_eq> & get_wmes();

    private:
      std::unordered_set<std::shared_ptr<const WME>, hash_deref<WME>, compare_deref_eq> m_wmes;
    };

  }

}

ZENI_RETE_LINKAGE std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Working_Memory &working_memory);

namespace std {

  template <> struct hash<Zeni::Rete::Working_Memory> {
    size_t operator()(const Zeni::Rete::Working_Memory &working_memory) const {
      return hash<std::unordered_set<std::shared_ptr<const Zeni::Rete::WME>, Zeni::hash_deref<Zeni::Rete::WME>, Zeni::compare_deref_eq>>()(working_memory.get_wmes());
    }
  };

}

#endif
