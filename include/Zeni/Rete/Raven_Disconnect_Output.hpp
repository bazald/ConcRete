#ifndef ZENI_RETE_RAVEN_DISCONNECT_OUTPUT_H
#define ZENI_RETE_RAVEN_DISCONNECT_OUTPUT_H

#include "Raven.hpp"

namespace Zeni {

  namespace Rete {

    class Raven_Disconnect_Output : public Rete::Raven {
      Raven_Disconnect_Output(const Raven_Disconnect_Output &) = delete;
      Raven_Disconnect_Output & operator=(const Raven_Disconnect_Output &) = delete;

    public:
      ZENI_RETE_LINKAGE Raven_Disconnect_Output(const std::shared_ptr<Pseudonode> &recipient, const std::shared_ptr<Network> &network, const std::shared_ptr<const Node> &output);

      void receive() const override;
    };

  }

}

#endif
