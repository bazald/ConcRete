#ifndef ZENI_RETE_RAVEN_DISCONNECT_GATE_H
#define ZENI_RETE_RAVEN_DISCONNECT_GATE_H

#include "Raven.hpp"

namespace Zeni::Rete {

  class Raven_Disconnect_Gate : public Rete::Raven {
    Raven_Disconnect_Gate(const Raven_Disconnect_Gate &) = delete;
    Raven_Disconnect_Gate & operator=(const Raven_Disconnect_Gate &) = delete;

  public:
    ZENI_RETE_LINKAGE Raven_Disconnect_Gate(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> output);

    void receive() const override;
  };

}

#endif
