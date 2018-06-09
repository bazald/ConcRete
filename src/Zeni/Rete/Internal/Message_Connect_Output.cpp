#include "Zeni/Rete/Internal/Message_Connect_Output.hpp"

#include "Zeni/Rete/Internal/Debug_Counters.hpp"
#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  Message_Connect_Output::Message_Connect_Output(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<Node> child_)
    : Message(recipient, network),
    child(child_)
  {
  }

  void Message_Connect_Output::receive() const {
    DEBUG_COUNTER_INCREMENT(g_connect_outputs_received, 1);
    std::dynamic_pointer_cast<Node>(get_recipient())->receive(*this);
  }

}
