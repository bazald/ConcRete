#include "Zeni/Rete/Message_Token.hpp"

#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  Message_Token::Message_Token(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> sender, const std::shared_ptr<const Token> token)
    : Message(recipient, network),
    m_token(token),
    m_sender(sender)
  {
  }

  const std::shared_ptr<const Node> & Message_Token::get_sender() const {
    return m_sender;
  }

  std::shared_ptr<const Token> Message_Token::get_Token() const {
    return m_token;
  }

}
