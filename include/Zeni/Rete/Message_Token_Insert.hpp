#ifndef ZENI_RETE_MESSAGE_TOKEN_INSERT_HPP
#define ZENI_RETE_MESSAGE_TOKEN_INSERT_HPP

#include "Message_Token.hpp"

namespace Zeni::Rete {

  class Message_Token_Insert : public Message_Token {
    Message_Token_Insert(const Message_Token_Insert &) = delete;
    Message_Token_Insert operator=(const Message_Token_Insert &) = delete;

  public:
    ZENI_RETE_LINKAGE Message_Token_Insert(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> sender, const std::shared_ptr<const Token> token);

    void receive() const override;
  };

}

#endif
