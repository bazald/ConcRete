#ifndef ZENI_RETE_MESSAGE_STATUS_NONEMPTY_HPP
#define ZENI_RETE_MESSAGE_STATUS_NONEMPTY_HPP

#include "Message.hpp"

namespace Zeni::Rete {

  class Message_Status_Nonempty : public Message {
    Message_Status_Nonempty(const Message_Status_Nonempty &) = delete;
    Message_Status_Nonempty & operator=(const Message_Status_Nonempty &) = delete;

  public:
    ZENI_RETE_LINKAGE Message_Status_Nonempty(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> child);

    void receive() const override;

    const std::shared_ptr<const Node> child;
  };

}

#endif
