#include "Zeni/Rete/Message_Decrement_Child_Count.hpp"

#include "Zeni/Rete/Internal/Debug_Counters.hpp"
#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  Message_Decrement_Child_Count::Message_Decrement_Child_Count(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network)
    : Message(recipient, network)
  {
  }

  void Message_Decrement_Child_Count::receive() const {
    DEBUG_COUNTER_INCREMENT(g_decrement_children_received, 1);
    std::dynamic_pointer_cast<Node>(get_recipient())->receive(*this);
  }

}
