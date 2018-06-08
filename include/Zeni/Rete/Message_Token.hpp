#ifndef ZENI_RETE_MESSAGE_TOKEN_HPP
#define ZENI_RETE_MESSAGE_TOKEN_HPP

#include "Message.hpp"

namespace Zeni::Rete {

  class Token;

  class Message_Token : public Rete::Message {
    Message_Token(const Message_Token &) = delete;
    Message_Token & operator=(const Message_Token &) = delete;

  public:
    ZENI_RETE_LINKAGE Message_Token(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> sender, const std::shared_ptr<const Token> token);

    ZENI_RETE_LINKAGE const std::shared_ptr<const Node> & get_sender() const;

    ZENI_RETE_LINKAGE std::shared_ptr<const Token> get_Token() const;

  private:
    const std::shared_ptr<const Node> m_sender;
    const std::shared_ptr<const Token> m_token;
  };

}

#endif
