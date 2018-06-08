#include "Zeni/Rete/Message_Token_Remove.hpp"

#include "Zeni/Rete/Internal/Debug_Counters.hpp"
#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  Message_Token_Remove::Message_Token_Remove(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> parent_, const std::shared_ptr<const Token> token_)
    : Message(recipient, network),
    parent(parent_),
    token(token_)
  {
  }

  void Message_Token_Remove::receive() const {
    DEBUG_COUNTER_INCREMENT(g_tokens_removed, 1);
    std::dynamic_pointer_cast<Node>(get_recipient())->receive(*this);
  }

}
