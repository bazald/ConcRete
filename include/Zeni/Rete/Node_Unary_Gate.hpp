#ifndef ZENI_RETE_NODE_UNARY_GATE_HPP
#define ZENI_RETE_NODE_UNARY_GATE_HPP

#include "Node.hpp"

namespace Zeni::Rete {

  class Token_Alpha;

  class Node_Unary_Gate : public Node {
    Node_Unary_Gate(const Node_Unary_Gate &) = delete;
    Node_Unary_Gate & operator=(const Node_Unary_Gate &) = delete;

    Node_Unary_Gate(const std::shared_ptr<Node> input);

  protected:
    ZENI_RETE_LINKAGE void send_disconnect_from_parents(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) override;

  public:
    ZENI_RETE_LINKAGE ~Node_Unary_Gate();

    ZENI_RETE_LINKAGE static std::shared_ptr<Node_Unary_Gate> Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> input);

    ZENI_RETE_LINKAGE bool receive(const Message_Connect_Gate &message) override;
    ZENI_RETE_LINKAGE bool receive(const Message_Connect_Output &message) override;
    ZENI_RETE_LINKAGE bool receive(const Message_Disconnect_Gate &message) override;
    ZENI_RETE_LINKAGE bool receive(const Message_Disconnect_Output &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Status_Empty &) override;
    ZENI_RETE_LINKAGE void receive(const Message_Status_Nonempty &) override;
    ZENI_RETE_LINKAGE void receive(const Message_Token_Insert &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Token_Remove &message) override;

    ZENI_RETE_LINKAGE bool operator==(const Node &rhs) const override;

    ZENI_RETE_LINKAGE std::pair<std::shared_ptr<Node>, std::shared_ptr<Node>> get_inputs() override;

    ZENI_RETE_LINKAGE std::shared_ptr<const Node> get_input() const;
    ZENI_RETE_LINKAGE std::shared_ptr<Node> get_input();

  private:
    const std::shared_ptr<Node> m_input;

    std::shared_ptr<Symbol_Constant_Int> m_nonempty_token_symbol = std::make_shared<Symbol_Constant_Int>(0);
    std::shared_ptr<Token_Alpha> m_nonempty_token;
  };

}

#endif
