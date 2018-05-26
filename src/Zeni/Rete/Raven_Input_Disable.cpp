#include "Zeni/Rete/Raven_Input_Disable.hpp"

#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  Raven_Input_Disable::Raven_Input_Disable(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> sender)
    : Raven(recipient, network, sender)
  {
  }

  void Raven_Input_Disable::receive() const {
    std::dynamic_pointer_cast<Node>(get_recipient())->receive(*this);
  }

}
