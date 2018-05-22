#include "Zeni/Rete/Raven_Token_Remove.hpp"

#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node.hpp"

namespace Zeni {

  namespace Rete {

    ZENI_RETE_LINKAGE Raven_Token_Remove::Raven_Token_Remove(const std::shared_ptr<Node> &recipient, const std::shared_ptr<Network> &network, const std::shared_ptr<const Node> &sender, const std::shared_ptr<const Token> &token)
      : Raven_Token(recipient, network, sender, token)
    {
    }

    void Raven_Token_Remove::receive() const {
      dynamic_cast<Node *>(get_recipient())->receive(*this);
    }

  }

}
