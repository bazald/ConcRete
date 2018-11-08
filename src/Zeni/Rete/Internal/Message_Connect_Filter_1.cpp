#include "Zeni/Rete/Internal/Message_Connect_Filter_1.hpp"

#include "Zeni/Rete/Internal/Debug_Counters.hpp"

namespace Zeni::Rete {

  Message_Connect_Filter_1::Message_Connect_Filter_1(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const Node_Filter_1::Filter_Layer_1_Snapshot snapshot_, const std::shared_ptr<const Node_Key> key_, const std::shared_ptr<Node> child_)
    : Message(recipient, network),
    snapshot(snapshot_),
    key(key_),
    child(child_)
  {
  }

  Message_Connect_Filter_1::Message_Connect_Filter_1(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, Node_Filter_1::Filter_Layer_1_Snapshot &&snapshot_, const std::shared_ptr<const Node_Key> key_, const std::shared_ptr<Node> child_)
    : Message(recipient, network),
    snapshot(std::move(snapshot_)),
    key(key_),
    child(child_)
  {
  }

  void Message_Connect_Filter_1::receive() const {
    DEBUG_COUNTER_INCREMENT(g_connect_outputs_received, 1);
    std::dynamic_pointer_cast<Node_Filter_1>(get_recipient())->receive(*this);
  }

}
