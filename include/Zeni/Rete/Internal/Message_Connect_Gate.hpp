#ifndef ZENI_RETE_MESSAGE_CONNECT_GATE_HPP
#define ZENI_RETE_MESSAGE_CONNECT_GATE_HPP

#include "Message.hpp"

namespace Zeni::Rete {

  class Message_Connect_Gate : public Message {
    Message_Connect_Gate(const Message_Connect_Gate &) = delete;
    Message_Connect_Gate & operator=(const Message_Connect_Gate &) = delete;

  public:
    ZENI_RETE_LINKAGE Message_Connect_Gate(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<Node> child);

    ZENI_RETE_LINKAGE const std::shared_ptr<const Node> & get_sender() const;

    void receive() const override;

    const std::shared_ptr<Node> child;
  };

}

#endif
