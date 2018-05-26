#ifndef ZENI_RETE_RAVEN_INPUT_DISABLE_H
#define ZENI_RETE_RAVEN_INPUT_DISABLE_H

#include "Raven.hpp"

namespace Zeni::Rete {

  class Raven_Input_Disable : public Rete::Raven {
    Raven_Input_Disable(const Raven_Input_Disable &) = delete;
    Raven_Input_Disable & operator=(const Raven_Input_Disable &) = delete;

  public:
    ZENI_RETE_LINKAGE Raven_Input_Disable(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> sender);

    void receive() const override;
  };

}

#endif
