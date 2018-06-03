#ifndef ZENI_RETE_NODE_PASSTHROUGH_GATED_H
#define ZENI_RETE_NODE_PASSTHROUGH_GATED_H

#include "Node_Passthrough.hpp"

namespace Zeni::Rete {

  class Node_Passthrough_Gated : public Node_Passthrough {
    Node_Passthrough_Gated(const Node_Passthrough &) = delete;
    Node_Passthrough_Gated & operator=(const Node_Passthrough_Gated &) = delete;

    Node_Passthrough_Gated(const std::shared_ptr<Node> input, const std::shared_ptr<Node> gate);

  protected:
    ZENI_RETE_LINKAGE void send_disconnect_from_parents(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const Locked_Node_Data &locked_node_data) override;

  public:
    ZENI_RETE_LINKAGE ~Node_Passthrough_Gated();

    ZENI_RETE_LINKAGE static std::shared_ptr<Node_Passthrough_Gated> Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> input, const std::shared_ptr<Node> gate);

    ZENI_RETE_LINKAGE std::shared_ptr<const Node> get_gate() const;
    ZENI_RETE_LINKAGE std::shared_ptr<Node> get_gate();

    ZENI_RETE_LINKAGE bool operator==(const Node &rhs) const override;

  private:
    std::shared_ptr<Node> m_gate;
  };

}

#endif
