#ifndef ZENI_RETE_MESSAGE_DISCONNECT_GATE_HPP
#define ZENI_RETE_MESSAGE_DISCONNECT_GATE_HPP

#include "Message.hpp"

namespace Zeni::Rete {

  class Message_Disconnect_Gate : public Message {
    Message_Disconnect_Gate(const Message_Disconnect_Gate &) = delete;
    Message_Disconnect_Gate & operator=(const Message_Disconnect_Gate &) = delete;

  public:
    ZENI_RETE_LINKAGE Message_Disconnect_Gate(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<Node> child, const bool decrement_output_count_);

    void receive() const override;

    const std::shared_ptr<Node> child;
    const bool decrement_output_count;
  };

}

#endif
