#ifndef ZENI_RETE_MESSAGE_CONNECT_PREDICATE_HPP
#define ZENI_RETE_MESSAGE_CONNECT_PREDICATE_HPP

#include "Message.hpp"
#include "../Node_Key.hpp"
#include "../Node_Predicate.hpp"

namespace Zeni::Rete {

  class Message_Connect_Predicate : public Message {
    Message_Connect_Predicate(const Message_Connect_Predicate &) = delete;
    Message_Connect_Predicate & operator=(const Message_Connect_Predicate &) = delete;

  public:
    ZENI_RETE_LINKAGE Message_Connect_Predicate(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const Node_Predicate::Predicate_Layer_Snapshot snapshot, const std::shared_ptr<Node> child);
    ZENI_RETE_LINKAGE Message_Connect_Predicate(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, Node_Predicate::Predicate_Layer_Snapshot &&snapshot, const std::shared_ptr<Node> child);

    void receive() const override;

    const Node_Predicate::Predicate_Layer_Snapshot snapshot;
    const std::shared_ptr<Node> child;
  };

}

#endif
