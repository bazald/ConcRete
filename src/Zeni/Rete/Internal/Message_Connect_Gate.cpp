#include "Zeni/Rete/Internal/Message_Connect_Gate.hpp"

#include "Zeni/Rete/Internal/Debug_Counters.hpp"
#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  Message_Connect_Gate::Message_Connect_Gate(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node::Node_Data_Snapshot> snapshot_, const std::shared_ptr<Node> child_)
    : Message(recipient, network),
    snapshot(snapshot_),
    child(child_)
  {
  }

  void Message_Connect_Gate::receive() const {
    DEBUG_COUNTER_INCREMENT(g_connect_gates_received, 1);
    std::dynamic_pointer_cast<Node>(get_recipient())->receive(*this);
  }

}
