#include "Zeni/Rete/Internal/Message_Connect_Join.hpp"

namespace Zeni::Rete {

  Message_Connect_Join::Message_Connect_Join(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const Node_Join::Join_Layer_Snapshot snapshot_, const std::shared_ptr<const Node_Key> key_, const std::shared_ptr<Node> child_)
    : Message(recipient, network),
    snapshot(snapshot_),
    key(key_),
    child(child_)
  {
  }

  Message_Connect_Join::Message_Connect_Join(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, Node_Join::Join_Layer_Snapshot &&snapshot_, const std::shared_ptr<const Node_Key> key_, const std::shared_ptr<Node> child_)
    : Message(recipient, network),
    snapshot(std::move(snapshot_)),
    key(key_),
    child(child_)
  {
  }

  void Message_Connect_Join::receive() const {
    std::dynamic_pointer_cast<Node_Join>(get_recipient())->receive(*this);
  }

}
