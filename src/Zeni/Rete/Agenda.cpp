#include "Zeni/Rete/Agenda.hpp"

#include "Zeni/Rete/Node_Action.hpp"

namespace Zeni {

  namespace Rete {

    void Agenda::insert_action(const std::shared_ptr<const Node_Action> &action, const std::shared_ptr<const Token> &wme_token) {
      //#ifdef DEBUG_OUTPUT
      //    std::cerr << "Inserting " << *wme_token << std::endl;std::tuple<std::shared_ptr<const Node_Action>, std::shared_ptr<const Token>, bool>, Zeni::Pool_Allocator<std::tuple<std::shared_ptr<const Node_Action>, std::shared_ptr<const Token>, bool>>
      //#endif
      agenda.insert(std::make_pair(action, wme_token));
      run();
    }

    void Agenda::insert_retraction(const std::shared_ptr<const Node_Action> &action, const std::shared_ptr<const Token> &wme_token) {
      const auto found = agenda.find(std::make_pair(action, wme_token));
      if (found != agenda.end())
        agenda.erase(found);
      else
        Node_Action_to_Agenda::retraction(*action)(*action, *wme_token);
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
      if (m_locked)
        return;
      ++m_locked;

      while (!agenda.empty()) {
        const auto front_iterator = agenda.begin();
        const auto front = *front_iterator;
        agenda.erase(front_iterator);
        Node_Action_to_Agenda::action(*front.first)(*front.first, *front.second);
      }

      assert(m_locked);
      --m_locked;
    }

  }

}
