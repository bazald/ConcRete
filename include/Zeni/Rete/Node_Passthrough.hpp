#ifndef ZENI_RETE_NODE_PASSTHROUGH_HPP
#define ZENI_RETE_NODE_PASSTHROUGH_HPP

#include "Internal/Node_Unary.hpp"

namespace Zeni::Rete {

  class Node_Passthrough : public Node_Unary {
    Node_Passthrough(const Node_Passthrough &) = delete;
    Node_Passthrough & operator=(const Node_Passthrough &) = delete;

  protected:
    Node_Passthrough(const std::shared_ptr<Node> input);
    Node_Passthrough(const size_t hash, const std::shared_ptr<Node> input);

  public:
    ZENI_RETE_LINKAGE ~Node_Passthrough();

    ZENI_RETE_LINKAGE static std::shared_ptr<Node_Passthrough> Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> input);

    ZENI_RETE_LINKAGE void receive(const Message_Token_Insert &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Token_Remove &message) override;

    ZENI_RETE_LINKAGE bool operator==(const Node &rhs) const override;
  };

}

#endif
