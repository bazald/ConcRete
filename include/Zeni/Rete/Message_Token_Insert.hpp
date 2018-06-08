#ifndef ZENI_RETE_MESSAGE_TOKEN_INSERT_HPP
#define ZENI_RETE_MESSAGE_TOKEN_INSERT_HPP

#include "Message.hpp"

namespace Zeni::Rete {

  class Token;

  class Message_Token_Insert : public Message {
    Message_Token_Insert(const Message_Token_Insert &) = delete;
    Message_Token_Insert operator=(const Message_Token_Insert &) = delete;

  public:
    ZENI_RETE_LINKAGE Message_Token_Insert(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> parent, const std::shared_ptr<const Token> token);

    void receive() const override;

    const std::shared_ptr<const Node> parent;
    const std::shared_ptr<const Token> token;
  };

}

#endif
