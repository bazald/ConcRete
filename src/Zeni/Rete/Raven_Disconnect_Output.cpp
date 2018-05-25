#include "Zeni/Rete/Raven_Disconnect_Output.hpp"

#include "Zeni/Rete/Node.hpp"

namespace Zeni {

  namespace Rete {

    Raven_Disconnect_Output::Raven_Disconnect_Output(const std::shared_ptr<Pseudonode> &recipient, const std::shared_ptr<Network> &network, const std::shared_ptr<const Node> &output)
      : Raven(recipient, network, output)
    {
    }

    void Raven_Disconnect_Output::receive() const {
      std::dynamic_pointer_cast<Pseudonode>(get_recipient())->receive(*this);
    }

  }

}
