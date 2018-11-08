#include "Zeni/Rete/Internal/Message_Token_Remove.hpp"

#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  Message_Token_Remove::Message_Token_Remove(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<Node> parent_, const std::shared_ptr<const Node_Key> key_, const std::shared_ptr<const Token> token_)
    : Message(recipient, network),
    parent(parent_),
    key(key_),
    token(token_)
  {
  }

  void Message_Token_Remove::receive() const {
    std::dynamic_pointer_cast<Node>(get_recipient())->receive(*this);
  }

}
