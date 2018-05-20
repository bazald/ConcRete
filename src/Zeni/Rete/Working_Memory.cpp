#include "Zeni/Rete/Working_Memory.hpp"

namespace Zeni {

  namespace Rete {

    const std::unordered_set<std::shared_ptr<const WME>, hash_deref<WME>, compare_deref_eq> & Working_Memory::get_wmes() const {
      return m_wmes;
    }

    std::unordered_set<std::shared_ptr<const WME>, hash_deref<WME>, compare_deref_eq> & Working_Memory::get_wmes() {
      return m_wmes;
    }

  }

}

std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Working_Memory &working_memory) {
  os << '{' << std::endl;
  for (const auto &wme : working_memory.get_wmes())
    os << "  " << *wme << std::endl;
  os << '}';
  return os;
}
