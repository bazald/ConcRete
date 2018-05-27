#ifndef ZENI_RETE_NODE_UNARY_GATE_H
#define ZENI_RETE_NODE_UNARY_GATE_H

#include "Node_Unary.hpp"

namespace Zeni::Rete {

  class Node_Unary_Gate : public Node_Unary {
    Node_Unary_Gate(const Node_Unary_Gate &) = delete;
    Node_Unary_Gate & operator=(const Node_Unary_Gate &) = delete;

    Node_Unary_Gate(const std::shared_ptr<Node> input);

  public:
    ZENI_RETE_LINKAGE static std::shared_ptr<Node_Unary_Gate> Create(const std::shared_ptr<Network> network, const std::shared_ptr<Node> input);

    ZENI_RETE_LINKAGE void receive(const Raven_Status_Empty &) override;
    ZENI_RETE_LINKAGE void receive(const Raven_Status_Nonempty &) override;
    ZENI_RETE_LINKAGE void receive(const Raven_Token_Insert &raven) override;
    ZENI_RETE_LINKAGE void receive(const Raven_Token_Remove &raven) override;

    ZENI_RETE_LINKAGE bool operator==(const Node &rhs) const override;
  };

}

#endif
