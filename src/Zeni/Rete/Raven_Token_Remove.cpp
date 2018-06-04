#include "Zeni/Rete/Raven_Token_Remove.hpp"

#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  Raven_Token_Remove::Raven_Token_Remove(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> sender, const std::shared_ptr<const Token> token)
    : Raven_Token(recipient, network, sender, token)
  {
  }

  void Raven_Token_Remove::receive() const {
    Zeni::Rete::Counters::g_tokens_removed.fetch_add(1, std::memory_order_acquire);
    std::dynamic_pointer_cast<Node>(get_recipient())->receive(*this);
  }

}
