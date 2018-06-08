#include "Zeni/Rete/Message_Disconnect_Gate.hpp"

#include "Zeni/Rete/Internal/Debug_Counters.hpp"
#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  Message_Disconnect_Gate::Message_Disconnect_Gate(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> child_, const bool decrement_output_count_)
    : Message(recipient, network),
    decrement_output_count(decrement_output_count_),
    child(child_)
  {
  }

  void Message_Disconnect_Gate::receive() const {
    DEBUG_COUNTER_INCREMENT(g_disconnect_gates_received, 1);
    std::dynamic_pointer_cast<Node>(get_recipient())->receive(*this);
  }

}
