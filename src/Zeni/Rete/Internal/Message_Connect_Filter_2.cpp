#include "Zeni/Rete/Internal/Message_Connect_Filter_2.hpp"

namespace Zeni::Rete {

  Message_Connect_Filter_2::Message_Connect_Filter_2(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const Node::Filter_Layer_Snapshot snapshot_, const std::shared_ptr<const Node_Key> key_, const std::shared_ptr<Node> child_)
    : Message(recipient, network),
    snapshot(snapshot_),
    key(key_),
    child(child_)
  {
  }

  Message_Connect_Filter_2::Message_Connect_Filter_2(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, Node::Filter_Layer_Snapshot &&snapshot_, const std::shared_ptr<const Node_Key> key_, const std::shared_ptr<Node> child_)
    : Message(recipient, network),
    snapshot(std::move(snapshot_)),
    key(key_),
    child(child_)
  {
  }

  void Message_Connect_Filter_2::receive() const {
    std::dynamic_pointer_cast<Node_Filter_2>(get_recipient())->receive(*this);
  }

}
