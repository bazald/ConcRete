#include "Zeni/Rete/Internal/Message_Connect_Filter_0.hpp"

namespace Zeni::Rete {

  Message_Connect_Filter_0::Message_Connect_Filter_0(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const Network::Filter_Layer_0_Snapshot snapshot_, const std::shared_ptr<const Node_Key> key_, const std::shared_ptr<Node> child_)
    : Message(recipient, network),
    snapshot(snapshot_),
    key(key_),
    child(child_)
  {
  }

  Message_Connect_Filter_0::Message_Connect_Filter_0(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, Network::Filter_Layer_0_Snapshot &&snapshot_, const std::shared_ptr<const Node_Key> key_, const std::shared_ptr<Node> child_)
    : Message(recipient, network),
    snapshot(std::move(snapshot_)),
    key(key_),
    child(child_)
  {
  }

  void Message_Connect_Filter_0::receive() const {
    std::dynamic_pointer_cast<Network>(get_recipient())->receive(*this);
  }

}
