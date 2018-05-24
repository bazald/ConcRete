#ifndef ZENI_RETE_PSEUDONODE_H
#define ZENI_RETE_PSEUDONODE_H

#include "Zeni/Concurrency/Maester.hpp"
#include "Zeni/Rete/Linkage.hpp"

namespace Zeni {

  namespace Rete {

    class Network;
    class Node;
    class Raven_Disconnect_Output;
    class Raven_Token_Insert;
    class Raven_Token_Remove;

    class Pseudonode : public Concurrency::Maester {
    public:
      /// Decrements the output count, potentially resulting in cascading disconnects
      ZENI_RETE_LINKAGE virtual void disconnect_output(const std::shared_ptr<Network> &network, const std::shared_ptr<const Node> &output) = 0;

      ZENI_RETE_LINKAGE void receive(Concurrency::Job_Queue &job_queue, const Concurrency::Raven &raven) override;
      ZENI_RETE_LINKAGE void receive(const Raven_Disconnect_Output &raven);
      /// Returns true if and only if the token matches and can be inserted (no antitokens present)
      ZENI_RETE_LINKAGE virtual bool receive(const Raven_Token_Insert &raven) = 0;
      /// Returns true if and only if the token matches and can be removed (at least one token present)
      ZENI_RETE_LINKAGE virtual bool receive(const Raven_Token_Remove &raven) = 0;
    };

  }

}

#endif
