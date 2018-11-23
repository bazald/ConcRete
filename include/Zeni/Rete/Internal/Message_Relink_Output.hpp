#ifndef ZENI_RETE_MESSAGE_RELINK_OUTPUT_HPP
#define ZENI_RETE_MESSAGE_RELINK_OUTPUT_HPP

#include "Message.hpp"
#include "../Node.hpp"
#include "../Node_Key.hpp"

namespace Zeni::Rete {

  class Message_Relink_Output : public Message {
    Message_Relink_Output(const Message_Relink_Output &) = delete;
    Message_Relink_Output & operator=(const Message_Relink_Output &) = delete;

  public:
    ZENI_RETE_LINKAGE Message_Relink_Output(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child);

    void receive() const override;

    const std::shared_ptr<const Node_Key> key;
    const std::shared_ptr<Node> child;
  };

}

#endif
