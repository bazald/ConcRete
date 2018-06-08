#ifndef ZENI_RETE_MESSAGE_DECREMENT_CHILD_COUNT_HPP
#define ZENI_RETE_MESSAGE_DECREMENT_CHILD_COUNT_HPP

#include "Message.hpp"

namespace Zeni::Rete {

  class Message_Decrement_Child_Count : public Rete::Message {
    Message_Decrement_Child_Count(const Message_Decrement_Child_Count &) = delete;
    Message_Decrement_Child_Count & operator=(const Message_Decrement_Child_Count &) = delete;

  public:
    ZENI_RETE_LINKAGE Message_Decrement_Child_Count(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network);

    void receive() const override;
  };

}

#endif
