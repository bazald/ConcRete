#ifndef ZENI_RETE_PSEUDONODE_H
#define ZENI_RETE_PSEUDONODE_H

#include "Zeni/Concurrency/Maester.hpp"
#include "Linkage.hpp"

#include <unordered_set>

namespace Zeni::Rete {

  class Network;
  class Node;
  class Raven_Disconnect_Output;
  class Raven_Token_Insert;
  class Raven_Token_Remove;

  class Pseudonode : public Concurrency::Maester {
  public:
    typedef std::unordered_set<std::shared_ptr<Node>> Outputs;

    /// Find an existing equivalent to output and return it, or return the new output if no equivalent exists
    ZENI_RETE_LINKAGE virtual std::shared_ptr<Node> connect_output(const std::shared_ptr<Network> network, const std::shared_ptr<Node> output) = 0;
    /// Decrements the output count, potentially resulting in cascading disconnects
    ZENI_RETE_LINKAGE virtual void disconnect_output(const std::shared_ptr<Network> network, const std::shared_ptr<const Node> output) = 0;

    ZENI_RETE_LINKAGE void receive(Concurrency::Job_Queue &job_queue, const std::shared_ptr<const Concurrency::Raven> raven) override;
    ZENI_RETE_LINKAGE void receive(const Raven_Disconnect_Output &raven);
    /// Returns true if and only if the token matches and can be inserted (no antitokens present)
    ZENI_RETE_LINKAGE virtual bool receive(const Raven_Token_Insert &raven) = 0;
    /// Returns true if and only if the token matches and can be removed (at least one token present)
    ZENI_RETE_LINKAGE virtual bool receive(const Raven_Token_Remove &raven) = 0;
  };

}

#endif
