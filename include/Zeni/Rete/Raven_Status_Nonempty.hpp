#ifndef ZENI_RETE_RAVEN_STATUS_NONEMPTY_H
#define ZENI_RETE_RAVEN_STATUS_NONEMPTY_H

#include "Raven.hpp"

namespace Zeni::Rete {

  class Raven_Status_Nonempty : public Rete::Raven {
    Raven_Status_Nonempty(const Raven_Status_Nonempty &) = delete;
    Raven_Status_Nonempty & operator=(const Raven_Status_Nonempty &) = delete;

  public:
    ZENI_RETE_LINKAGE Raven_Status_Nonempty(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> sender);

    void receive() const override;
  };

}

#endif
