#include "Zeni/Rete/Message_Disconnect_Output.hpp"

#include "Zeni/Rete/Internal/Debug_Counters.hpp"
#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  Message_Disconnect_Output::Message_Disconnect_Output(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> output, const bool decrement_output_count_)
    : Message(recipient, network, output),
    decrement_output_count(decrement_output_count_)
  {
  }

  void Message_Disconnect_Output::receive() const {
    if (decrement_output_count)
      DEBUG_COUNTER_INCREMENT(g_disconnect_output_and_decrements_received, 1);
    else
      DEBUG_COUNTER_INCREMENT(g_disconnect_output_but_nodecrements_received, 1);
    std::dynamic_pointer_cast<Node>(get_recipient())->receive(*this);
  }

}
