#include "Zeni/Rete/Raven_Status_Empty.hpp"

#include "Zeni/Rete/Internal/Debug_Counters.hpp"
#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  Raven_Status_Empty::Raven_Status_Empty(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> sender)
    : Raven(recipient, network, sender)
  {
  }

  void Raven_Status_Empty::receive() const {
    DEBUG_COUNTER_INCREMENT(g_empties_received, 1);
    std::dynamic_pointer_cast<Node>(get_recipient())->receive(*this);
  }

}
