#include "Zeni/Rete/Internal/Message_Connect_Predicate.hpp"

namespace Zeni::Rete {

  Message_Connect_Predicate::Message_Connect_Predicate(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const Node_Predicate::Predicate_Layer_Snapshot snapshot_, const std::shared_ptr<Node> child_)
    : Message(recipient, network),
    snapshot(snapshot_),
    child(child_)
  {
  }

  Message_Connect_Predicate::Message_Connect_Predicate(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, Node_Predicate::Predicate_Layer_Snapshot &&snapshot_, const std::shared_ptr<Node> child_)
    : Message(recipient, network),
    snapshot(std::move(snapshot_)),
    child(child_)
  {
  }

  void Message_Connect_Predicate::receive() const {
    std::dynamic_pointer_cast<Node_Predicate>(get_recipient())->receive(*this);
  }

}
