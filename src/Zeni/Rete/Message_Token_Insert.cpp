#include "Zeni/Rete/Message_Token_Insert.hpp"

#include "Zeni/Rete/Internal/Debug_Counters.hpp"
#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  Message_Token_Insert::Message_Token_Insert(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> sender, const std::shared_ptr<const Token> token)
    : Message_Token(recipient, network, sender, token)
  {
  }

  void Message_Token_Insert::receive() const {
    DEBUG_COUNTER_INCREMENT(g_tokens_inserted, 1);
    std::dynamic_pointer_cast<Node>(get_recipient())->receive(*this);
  }

}
