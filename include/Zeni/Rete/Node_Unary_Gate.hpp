#ifndef ZENI_RETE_NODE_UNARY_GATE_HPP
#define ZENI_RETE_NODE_UNARY_GATE_HPP

#include "Node_Unary.hpp"

namespace Zeni::Rete {

  class Node_Unary_Gate : public Node_Unary {
    Node_Unary_Gate(const Node_Unary_Gate &) = delete;
    Node_Unary_Gate & operator=(const Node_Unary_Gate &) = delete;

    Node_Unary_Gate(const std::shared_ptr<Node> input);

  protected:
    ZENI_RETE_LINKAGE void send_disconnect_from_parents(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const Locked_Node_Data &locked_node_data) override;

  public:
    ZENI_RETE_LINKAGE ~Node_Unary_Gate();

    ZENI_RETE_LINKAGE static std::shared_ptr<Node_Unary_Gate> Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> input);

    ZENI_RETE_LINKAGE void receive(const Raven_Disconnect_Gate &raven) override;
    ZENI_RETE_LINKAGE void receive(const Raven_Disconnect_Output &raven) override;
    ZENI_RETE_LINKAGE void receive(const Raven_Status_Empty &) override;
    ZENI_RETE_LINKAGE void receive(const Raven_Status_Nonempty &) override;
    ZENI_RETE_LINKAGE void receive(const Raven_Token_Insert &raven) override;
    ZENI_RETE_LINKAGE void receive(const Raven_Token_Remove &raven) override;

    ZENI_RETE_LINKAGE bool operator==(const Node &rhs) const override;
  };

}

#endif
