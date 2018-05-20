#ifndef RETE_AGENDA_H
#define RETE_AGENDA_H

#include "Node.h"

#include <cassert>
#include <cstddef>
#include <functional>
#include <list>

namespace Zeni {

  namespace Rete {

    class Node;

    class Agenda {
      Agenda(Agenda &);
      Agenda & operator=(Agenda &);

    public:
      class ZENI_RETE_LINKAGE Locker {
        Locker(const Locker &rhs);
        Locker & operator=(const Locker &rhs);

      public:
        Locker(Agenda &agenda)
          : m_agenda(agenda)
        {
          m_agenda.lock();
        }

        ~Locker() {
          m_agenda.unlock();
          m_agenda.run();
        }

      private:
        Agenda & m_agenda;
      };

      ZENI_RETE_LINKAGE Agenda() {}

      ZENI_RETE_LINKAGE void insert_action(const std::shared_ptr<const Node_Action> &action, const std::shared_ptr<const Token> &token);
      ZENI_RETE_LINKAGE void insert_retraction(const std::shared_ptr<const Node_Action> &action, const std::shared_ptr<const Token> &token);

      ZENI_RETE_LINKAGE void lock();
      ZENI_RETE_LINKAGE void unlock();
      ZENI_RETE_LINKAGE void run();

    private:
      std::unordered_set<std::pair<std::shared_ptr<const Node_Action>, std::shared_ptr<const Token>>> agenda;
      int64_t m_locked = 0;
      int64_t m_manually_locked = 0;
    };

  }

}

#endif
