#ifndef ZENI_RETE_MESSAGE_STATUS_EMPTY_HPP
#define ZENI_RETE_MESSAGE_STATUS_EMPTY_HPP

#include "Message.hpp"

namespace Zeni::Rete {

  class Message_Status_Empty : public Rete::Message {
    Message_Status_Empty(const Message_Status_Empty &) = delete;
    Message_Status_Empty & operator=(const Message_Status_Empty &) = delete;

  public:
    ZENI_RETE_LINKAGE Message_Status_Empty(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> sender);

    void receive() const override;
  };

}

#endif
