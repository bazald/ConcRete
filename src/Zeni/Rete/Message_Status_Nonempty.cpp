#include "Zeni/Rete/Message_Status_Nonempty.hpp"

#include "Zeni/Rete/Internal/Debug_Counters.hpp"
#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  Message_Status_Nonempty::Message_Status_Nonempty(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> child_)
    : Message(recipient, network),
    child(child_)
  {
  }

  void Message_Status_Nonempty::receive() const {
    DEBUG_COUNTER_INCREMENT(g_nonempties_received, 1);
    std::dynamic_pointer_cast<Node>(get_recipient())->receive(*this);
  }

}
