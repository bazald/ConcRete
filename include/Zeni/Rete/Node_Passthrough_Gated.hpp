#ifndef ZENI_RETE_NODE_PASSTHROUGH_GATED_H
#define ZENI_RETE_NODE_PASSTHROUGH_GATED_H

#include "Node_Passthrough.hpp"

namespace Zeni::Rete {

  class Node_Passthrough_Gated : public Node_Passthrough {
    Node_Passthrough_Gated(const Node_Passthrough &) = delete;
    Node_Passthrough_Gated & operator=(const Node_Passthrough_Gated &) = delete;

    Node_Passthrough_Gated(const std::shared_ptr<Node> input, const std::shared_ptr<Node> gate);

  public:
    ZENI_RETE_LINKAGE static std::shared_ptr<Node_Passthrough_Gated> Create(const std::shared_ptr<Network> network, const std::shared_ptr<Node> input, const std::shared_ptr<Node> gate);

    ZENI_RETE_LINKAGE std::shared_ptr<const Node> get_gate() const;
    ZENI_RETE_LINKAGE std::shared_ptr<Node> get_gate();

    ZENI_RETE_LINKAGE bool operator==(const Node &rhs) const override;

  private:
    std::shared_ptr<Node> m_gate;
  };

}

#endif
