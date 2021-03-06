#ifndef ZENI_RETE_MESSAGE_CONNECT_FILTER_0_HPP
#define ZENI_RETE_MESSAGE_CONNECT_FILTER_0_HPP

#include "Message.hpp"
#include "../Node_Key.hpp"
#include "../Network.hpp"

namespace Zeni::Rete {

  class Message_Connect_Filter_0 : public Message {
    Message_Connect_Filter_0(const Message_Connect_Filter_0 &) = delete;
    Message_Connect_Filter_0 & operator=(const Message_Connect_Filter_0 &) = delete;

  public:
    ZENI_RETE_LINKAGE Message_Connect_Filter_0(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const Node::Filter_Layer_Snapshot snapshot, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child);
    ZENI_RETE_LINKAGE Message_Connect_Filter_0(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, Node::Filter_Layer_Snapshot &&snapshot, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child);

    void receive() const override;

    const Node::Filter_Layer_Snapshot snapshot;
    const std::shared_ptr<const Node_Key> key;
    const std::shared_ptr<Node> child;
  };

}

#endif
