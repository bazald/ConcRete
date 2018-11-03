#ifndef ZENI_RETE_NODE_JOIN_HPP
#define ZENI_RETE_NODE_JOIN_HPP

#include "Internal/Node_Binary.hpp"

namespace Zeni::Rete {

  class Node_Join : public Node_Binary {
    Node_Join(const Node_Join &) = delete;
    Node_Join & operator=(const Node_Join &) = delete;

  protected:
    Node_Join(const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const Variable_Bindings &variable_bindings);
    Node_Join(const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, Variable_Bindings &&variable_bindings);
    Node_Join(const size_t hash, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const Variable_Bindings &variable_bindings);
    Node_Join(const size_t hash, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, Variable_Bindings &&variable_bindings);

  public:
    ZENI_RETE_LINKAGE ~Node_Join();

    ZENI_RETE_LINKAGE static std::shared_ptr<Node_Join> Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const Variable_Bindings &variable_bindings);
    ZENI_RETE_LINKAGE static std::shared_ptr<Node_Join> Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, Variable_Bindings &&variable_bindings);

    ZENI_RETE_LINKAGE void receive(const Message_Token_Insert &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Token_Remove &message) override;

    ZENI_RETE_LINKAGE bool operator==(const Node &rhs) const override;

  private:
    static std::shared_ptr<Node_Join> connect_created(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const std::shared_ptr<Node_Join> created);
    bool test_variable_bindings(const std::shared_ptr<const Token> token_left, const std::shared_ptr<const Token> token_right) const;

    const Variable_Bindings m_variable_bindings;
  };

}

#endif
