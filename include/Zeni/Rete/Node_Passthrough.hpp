#ifndef ZENI_RETE_NODE_PASSTHROUGH_H
#define ZENI_RETE_NODE_PASSTHROUGH_H

#include "Node_Unary.hpp"

namespace Zeni::Rete {

  class Node_Passthrough : public Node_Unary {
    Node_Passthrough(const Node_Passthrough &) = delete;
    Node_Passthrough & operator=(const Node_Passthrough &) = delete;

    Node_Passthrough(const std::shared_ptr<Node> input);

  public:
    ZENI_RETE_LINKAGE static std::shared_ptr<Node_Passthrough> Create(const std::shared_ptr<Network> network, const std::shared_ptr<Node> input);

    ZENI_RETE_LINKAGE bool receive(const Raven_Token_Insert &raven) override;
    ZENI_RETE_LINKAGE bool receive(const Raven_Token_Remove &raven) override;

    ZENI_RETE_LINKAGE bool operator==(const Node &rhs) const override;
  };

}

#endif
