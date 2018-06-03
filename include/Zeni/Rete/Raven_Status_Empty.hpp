#ifndef ZENI_RETE_RAVEN_STATUS_EMPTY_HPP
#define ZENI_RETE_RAVEN_STATUS_EMPTY_HPP

#include "Raven.hpp"

namespace Zeni::Rete {

  class Raven_Status_Empty : public Rete::Raven {
    Raven_Status_Empty(const Raven_Status_Empty &) = delete;
    Raven_Status_Empty & operator=(const Raven_Status_Empty &) = delete;

  public:
    ZENI_RETE_LINKAGE Raven_Status_Empty(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> sender);

    void receive() const override;
  };

}

#endif
