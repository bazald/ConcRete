#ifndef ZENI_RETE_MESSAGE_CONNECT_FILTER_1_HPP
#define ZENI_RETE_MESSAGE_CONNECT_FILTER_1_HPP

#include "Message.hpp"
#include "../Node_Filter_1.hpp"
#include "../Node_Key.hpp"

namespace Zeni::Rete {

  class Message_Connect_Filter_1 : public Message {
    Message_Connect_Filter_1(const Message_Connect_Filter_1 &) = delete;
    Message_Connect_Filter_1 & operator=(const Message_Connect_Filter_1 &) = delete;

  public:
    ZENI_RETE_LINKAGE Message_Connect_Filter_1(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const Node_Filter_1::Filter_Layer_1_Snapshot snapshot, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child);
    ZENI_RETE_LINKAGE Message_Connect_Filter_1(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, Node_Filter_1::Filter_Layer_1_Snapshot &&snapshot, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child);

    void receive() const override;

    const Node_Filter_1::Filter_Layer_1_Snapshot snapshot;
    const std::shared_ptr<const Node_Key> key;
    const std::shared_ptr<Node> child;
  };

}

#endif
