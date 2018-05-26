#include "Zeni/Rete/Raven_Input_Enable.hpp"

#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  Raven_Input_Enable::Raven_Input_Enable(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> sender)
    : Raven(recipient, network, sender)
  {
  }

  void Raven_Input_Enable::receive() const {
    std::dynamic_pointer_cast<Node>(get_recipient())->receive(*this);
  }

}
