#ifndef ZENI_RETE_RAVEN_CONNECT_GATE_HPP
#define ZENI_RETE_RAVEN_CONNECT_GATE_HPP

#include "Raven.hpp"

namespace Zeni::Rete {

  class Raven_Connect_Gate : public Rete::Raven {
    Raven_Connect_Gate(const Raven_Connect_Gate &) = delete;
    Raven_Connect_Gate & operator=(const Raven_Connect_Gate &) = delete;

  public:
    ZENI_RETE_LINKAGE Raven_Connect_Gate(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> output);

    void receive() const override;
  };

}

#endif
