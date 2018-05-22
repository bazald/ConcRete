#include "Zeni/Rete/Raven_Token_Insert.hpp"

#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node.hpp"

namespace Zeni {

  namespace Rete {

    ZENI_RETE_LINKAGE Raven_Token_Insert::Raven_Token_Insert(const std::shared_ptr<Node> &recipient, const std::shared_ptr<Network> &network, const std::shared_ptr<const Node> &sender, const std::shared_ptr<const Token> &token)
      : Raven_Token(recipient, network, sender, token)
    {
    }

    void Raven_Token_Insert::receive() const {
      dynamic_cast<Node *>(get_recipient())->receive(*this);
    }

  }

}
