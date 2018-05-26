#ifndef ZENI_RETE_RAVEN_CONNECT_OUTPUT_H
#define ZENI_RETE_RAVEN_CONNECT_OUTPUT_H

#include "Raven.hpp"

namespace Zeni::Rete {

  class Raven_Connect_Output : public Rete::Raven {
    Raven_Connect_Output(const Raven_Connect_Output &) = delete;
    Raven_Connect_Output & operator=(const Raven_Connect_Output &) = delete;

  public:
    ZENI_RETE_LINKAGE Raven_Connect_Output(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> output);

    void receive() const override;
  };

}

#endif
