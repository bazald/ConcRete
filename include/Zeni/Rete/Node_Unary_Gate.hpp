#ifndef ZENI_RETE_NODE_UNARY_GATE_HPP
#define ZENI_RETE_NODE_UNARY_GATE_HPP

#include "Node.hpp"

namespace Zeni::Rete {

  class Node_Unary_Gate : public Node {
    Node_Unary_Gate(const Node_Unary_Gate &) = delete;
    Node_Unary_Gate & operator=(const Node_Unary_Gate &) = delete;

    Node_Unary_Gate(const std::shared_ptr<Node> input);

  protected:
    class Locked_Node_Unary_Gate_Data_Const;
    class Locked_Node_Unary_Gate_Data;

    class Unlocked_Node_Unary_Gate_Data {
      Unlocked_Node_Unary_Gate_Data(const Unlocked_Node_Unary_Gate_Data &) = delete;
      Unlocked_Node_Unary_Gate_Data & operator=(const Unlocked_Node_Unary_Gate_Data &) = delete;

      friend Locked_Node_Unary_Gate_Data_Const;
      friend Locked_Node_Unary_Gate_Data;

    public:
      Unlocked_Node_Unary_Gate_Data();

    private:
      int64_t m_input_tokens = 0;
    };

    class Locked_Node_Unary_Gate_Data_Const {
      Locked_Node_Unary_Gate_Data_Const(const Locked_Node_Unary_Gate_Data_Const &) = delete;
      Locked_Node_Unary_Gate_Data_Const & operator=(const Locked_Node_Unary_Gate_Data_Const &) = delete;

      friend Locked_Node_Unary_Gate_Data;

    public:
      ZENI_RETE_LINKAGE Locked_Node_Unary_Gate_Data_Const(const Node_Unary_Gate * node, const Locked_Node_Data_Const &node_data);

      ZENI_RETE_LINKAGE int64_t get_input_tokens() const;

    private:
      const std::shared_ptr<const Unlocked_Node_Unary_Gate_Data> m_data;
    };

    class Locked_Node_Unary_Gate_Data : public Locked_Node_Unary_Gate_Data_Const {
      Locked_Node_Unary_Gate_Data(const Locked_Node_Unary_Gate_Data &) = delete;
      Locked_Node_Unary_Gate_Data & operator=(const Locked_Node_Unary_Gate_Data &) = delete;

    public:
      ZENI_RETE_LINKAGE Locked_Node_Unary_Gate_Data(Node_Unary_Gate * node, const Locked_Node_Data &node_data);

      ZENI_RETE_LINKAGE int64_t & modify_input_tokens();

    private:
      const std::shared_ptr<Unlocked_Node_Unary_Gate_Data> m_data;
    };

    ZENI_RETE_LINKAGE void send_disconnect_from_parents(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) override;

  public:
    ZENI_RETE_LINKAGE ~Node_Unary_Gate();

    ZENI_RETE_LINKAGE static std::shared_ptr<Node_Unary_Gate> Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> input);

    ZENI_RETE_LINKAGE void receive(const Message_Connect_Gate &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Connect_Output &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Disconnect_Gate &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Disconnect_Output &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Status_Empty &) override;
    ZENI_RETE_LINKAGE void receive(const Message_Status_Nonempty &) override;
    ZENI_RETE_LINKAGE void receive(const Message_Token_Insert &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Token_Remove &message) override;

    ZENI_RETE_LINKAGE bool operator==(const Node &rhs) const override;

    ZENI_RETE_LINKAGE std::pair<std::shared_ptr<Node>, std::shared_ptr<Node>> get_inputs() override;

    ZENI_RETE_LINKAGE std::shared_ptr<const Node> get_input() const;
    ZENI_RETE_LINKAGE std::shared_ptr<Node> get_input();

  private:
    std::shared_ptr<Unlocked_Node_Unary_Gate_Data> m_unlocked_node_unary_gate_data;
    const std::shared_ptr<Node> m_input;
  };

}

#endif
