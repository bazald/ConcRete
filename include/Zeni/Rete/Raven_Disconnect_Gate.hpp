#ifndef ZENI_RETE_RAVEN_DISCONNECT_GATE_H
#define ZENI_RETE_RAVEN_DISCONNECT_GATE_H

#include "Raven.hpp"

#include <vector>

namespace Zeni::Rete {

  class Raven_Disconnect_Gate : public Rete::Raven {
    Raven_Disconnect_Gate(const Raven_Disconnect_Gate &) = delete;
    Raven_Disconnect_Gate & operator=(const Raven_Disconnect_Gate &) = delete;

  public:
    ZENI_RETE_LINKAGE Raven_Disconnect_Gate(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> output, const bool decrement_output_count_);
    ZENI_RETE_LINKAGE Raven_Disconnect_Gate(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> output, const bool decrement_output_count_, const std::vector<std::shared_ptr<Node>> &forwards_);
    ZENI_RETE_LINKAGE Raven_Disconnect_Gate(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> output, const bool decrement_output_count_, std::vector<std::shared_ptr<Node>> &&forwards_);

    void receive() const override;

    const bool decrement_output_count;

    const std::vector<std::shared_ptr<Node>> forwards;
  };

}

#endif
