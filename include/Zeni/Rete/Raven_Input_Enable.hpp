#ifndef ZENI_RETE_RAVEN_INPUT_ENABLE_H
#define ZENI_RETE_RAVEN_INPUT_ENABLE_H

#include "Raven.hpp"

namespace Zeni::Rete {

  class Raven_Input_Enable : public Rete::Raven {
    Raven_Input_Enable(const Raven_Input_Enable &) = delete;
    Raven_Input_Enable & operator=(const Raven_Input_Enable &) = delete;

  public:
    ZENI_RETE_LINKAGE Raven_Input_Enable(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> sender);

    void receive() const override;
  };

}

#endif
