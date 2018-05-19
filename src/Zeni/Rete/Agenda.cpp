#include "agenda.h"

#include "rete_action.h"

namespace Rete {

  void Agenda::insert_action(const Rete_Action_Ptr_C &action, const WME_Token_Ptr_C &wme_token) {
//#ifdef DEBUG_OUTPUT
//    std::cerr << "Inserting " << *wme_token << std::endl;std::tuple<Rete_Action_Ptr_C, WME_Token_Ptr_C, bool>, Zeni::Pool_Allocator<std::tuple<Rete_Action_Ptr_C, WME_Token_Ptr_C, bool>>
//#endif
    agenda.insert(std::make_pair(action, wme_token));
    run();
  }

  void Agenda::insert_retraction(const Rete_Action_Ptr_C &action, const WME_Token_Ptr_C &wme_token) {
    const auto found = agenda.find(std::make_pair(action, wme_token));
    if(found != agenda.end())
      agenda.erase(found);
    else
      Rete_Action_to_Agenda::retraction(*action)(*action, *wme_token);
  }

  void Agenda::lock() {
    ++m_locked;
    ++m_manually_locked;
  }

  void Agenda::unlock() {
    assert(m_manually_locked);
    --m_locked;
    --m_manually_locked;
  }

  void Agenda::run() {
    if(m_locked)
      return;
    ++m_locked;

    while(!agenda.empty()) {
      const auto front_iterator = agenda.begin();
      const auto front = *front_iterator;
      agenda.erase(front_iterator);
      Rete_Action_to_Agenda::action(*front.first)(*front.first, *front.second);
    }

    assert(m_locked);
    --m_locked;
  }

}
