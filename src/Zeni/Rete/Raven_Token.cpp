#include "Zeni/Rete/Raven_Token.hpp"

#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  Raven_Token::Raven_Token(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> sender, const std::shared_ptr<const Token> token)
    : Raven(recipient, network, sender), m_token(token)
  {
  }

  std::shared_ptr<const Token> Raven_Token::get_Token() const {
    return m_token;
  }

}
