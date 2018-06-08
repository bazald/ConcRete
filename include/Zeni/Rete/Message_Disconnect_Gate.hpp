#ifndef ZENI_RETE_MESSAGE_DISCONNECT_GATE_HPP
#define ZENI_RETE_MESSAGE_DISCONNECT_GATE_HPP

#include "Message.hpp"

#include <vector>

namespace Zeni::Rete {

  class Message_Disconnect_Gate : public Rete::Message {
    Message_Disconnect_Gate(const Message_Disconnect_Gate &) = delete;
    Message_Disconnect_Gate & operator=(const Message_Disconnect_Gate &) = delete;

  public:
    ZENI_RETE_LINKAGE Message_Disconnect_Gate(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> output, const bool decrement_output_count_);

    void receive() const override;

    const bool decrement_output_count;
  };

}

#endif
