#ifndef ZENI_RETE_NODE_JOIN_NEGATION_HPP
#define ZENI_RETE_NODE_JOIN_NEGATION_HPP

#include "Internal/Node_Binary.hpp"

namespace Zeni::Rete {

  class Node_Join_Negation : public Node_Binary {
    Node_Join_Negation(const Node_Join_Negation &) = delete;
    Node_Join_Negation & operator=(const Node_Join_Negation &) = delete;

    Node_Join_Negation(const std::shared_ptr<const Node_Key> node_key_left, const std::shared_ptr<const Node_Key> node_key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const Variable_Bindings &variable_bindings);
    Node_Join_Negation(const std::shared_ptr<const Node_Key> node_key_left, const std::shared_ptr<const Node_Key> node_key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, Variable_Bindings &&variable_bindings);

  public:
    ZENI_RETE_LINKAGE static std::shared_ptr<Node_Join_Negation> Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> node_key_left, const std::shared_ptr<const Node_Key> node_key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const Variable_Bindings &variable_bindings);
    ZENI_RETE_LINKAGE static std::shared_ptr<Node_Join_Negation> Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> node_key_left, const std::shared_ptr<const Node_Key> node_key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, Variable_Bindings &&variable_bindings);

    ZENI_RETE_LINKAGE void receive(const Message_Token_Insert &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Token_Remove &message) override;

    ZENI_RETE_LINKAGE bool operator==(const Node &rhs) const override;
  };

}

#endif
