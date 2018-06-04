#include "Zeni/Rete/Raven_Status_Nonempty.hpp"

#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  Raven_Status_Nonempty::Raven_Status_Nonempty(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> sender)
    : Raven(recipient, network, sender)
  {
  }

  void Raven_Status_Nonempty::receive() const {
    Zeni::Rete::Counters::g_nonempties_received.fetch_add(1, std::memory_order_acquire);
    std::dynamic_pointer_cast<Node>(get_recipient())->receive(*this);
  }

}
