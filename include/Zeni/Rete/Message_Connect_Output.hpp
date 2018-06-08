#ifndef ZENI_RETE_MESSAGE_CONNECT_OUTPUT_HPP
#define ZENI_RETE_MESSAGE_CONNECT_OUTPUT_HPP

#include "Message.hpp"

namespace Zeni::Rete {

  class Message_Connect_Output : public Message {
    Message_Connect_Output(const Message_Connect_Output &) = delete;
    Message_Connect_Output & operator=(const Message_Connect_Output &) = delete;

  public:
    ZENI_RETE_LINKAGE Message_Connect_Output(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<Node> child);

    ZENI_RETE_LINKAGE const std::shared_ptr<const Node> & get_sender() const;

    void receive() const override;

    const std::shared_ptr<Node> child;
  };

}

#endif
