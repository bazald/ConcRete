#ifndef ZENI_RETE_MESSAGE_CONNECT_JOIN_HPP
#define ZENI_RETE_MESSAGE_CONNECT_JOIN_HPP

#include "Message.hpp"
#include "../Node_Join.hpp"
#include "../Node_Key.hpp"

namespace Zeni::Rete {

  class Message_Connect_Join : public Message {
    Message_Connect_Join(const Message_Connect_Join &) = delete;
    Message_Connect_Join & operator=(const Message_Connect_Join &) = delete;

  public:
    ZENI_RETE_LINKAGE Message_Connect_Join(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const Node_Join::Join_Layer_Snapshot snapshot, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child);
    ZENI_RETE_LINKAGE Message_Connect_Join(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, Node_Join::Join_Layer_Snapshot &&snapshot, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child);

    void receive() const override;

    const Node_Join::Join_Layer_Snapshot snapshot;
    const std::shared_ptr<const Node_Key> key;
    const std::shared_ptr<Node> child;
  };

}

#endif
