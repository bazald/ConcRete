#include "Zeni/Rete/Raven_Disconnect_Gate.hpp"

#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  Raven_Disconnect_Gate::Raven_Disconnect_Gate(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> output, const bool decrement_output_count_)
    : Raven(recipient, network, output),
    decrement_output_count(decrement_output_count_)
  {
  }

  void Raven_Disconnect_Gate::receive() const {
    std::dynamic_pointer_cast<Node>(get_recipient())->receive(*this);
  }

}
