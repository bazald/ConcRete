#include "Zeni/Rete/Raven_Disconnect_Output.hpp"

#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  Raven_Disconnect_Output::Raven_Disconnect_Output(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> output, const bool decrement_output_count_)
    : Raven(recipient, network, output),
    decrement_output_count(decrement_output_count_)
  {
  }

  void Raven_Disconnect_Output::receive() const {
    if (decrement_output_count)
      Zeni::Rete::Counters::g_disconnect_output_and_decrements_received.fetch_add(1, std::memory_order_acquire);
    else
      Zeni::Rete::Counters::g_disconnect_output_but_nodecrements_received.fetch_add(1, std::memory_order_acquire);
    std::dynamic_pointer_cast<Node>(get_recipient())->receive(*this);
  }

}
