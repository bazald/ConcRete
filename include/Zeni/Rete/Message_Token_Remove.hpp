#ifndef ZENI_RETE_MESSAGE_TOKEN_REMOVE_HPP
#define ZENI_RETE_MESSAGE_TOKEN_REMOVE_HPP

#include "Message_Token.hpp"

namespace Zeni::Rete {

  class Message_Token_Remove : public Message_Token {
    Message_Token_Remove(const Message_Token_Remove &) = delete;
    Message_Token_Remove & operator=(const Message_Token_Remove &) = delete;

  public:
    ZENI_RETE_LINKAGE Message_Token_Remove(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> sender, const std::shared_ptr<const Token> token);

    void receive() const override;
  };

}

#endif
