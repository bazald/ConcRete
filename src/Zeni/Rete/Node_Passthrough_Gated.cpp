#include "Zeni/Rete/Node_Passthrough_Gated.hpp"

#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Unary_Gate.hpp"
#include "Zeni/Rete/Raven_Decrement_Output_Count.hpp"
#include "Zeni/Rete/Raven_Connect_Output.hpp"
#include "Zeni/Rete/Raven_Disconnect_Output.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Passthrough_Gated::Node_Passthrough_Gated(const std::shared_ptr<Node> input, const std::shared_ptr<Node> gate)
    : Node_Passthrough(input),
    m_gate(gate)
  {
  }

  void Node_Passthrough_Gated::send_disconnect_from_parents(const std::shared_ptr<Network> network, const Locked_Node_Data &locked_node_data) {
    {
      Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

      if (locked_node_unary_data.get_input_tokens().empty())
        network->get_Job_Queue()->give_one(std::make_shared<Raven_Decrement_Output_Count>(get_input(), network, shared_from_this()));
      else
        network->get_Job_Queue()->give_one(std::make_shared<Raven_Disconnect_Output>(get_input(), network, shared_from_this(), true));
    }

    network->get_Job_Queue()->give_one(std::make_shared<Raven_Disconnect_Output>(m_gate, network, shared_from_this(), true));
  }

  Node_Passthrough_Gated::~Node_Passthrough_Gated() {
  }

  std::shared_ptr<Node_Passthrough_Gated> Node_Passthrough_Gated::Create(const std::shared_ptr<Network> network, const std::shared_ptr<Node> input, const std::shared_ptr<Node> gate) {
    class Friendly_Node_Passthrough_Gated : public Node_Passthrough_Gated {
    public:
      Friendly_Node_Passthrough_Gated(const std::shared_ptr<Node> &input, const std::shared_ptr<Node> gate) : Node_Passthrough_Gated(input, gate) {}
    };

    const auto created = std::make_shared<Friendly_Node_Passthrough_Gated>(input, gate);
    const auto connected = std::dynamic_pointer_cast<Node_Passthrough_Gated>(input->connect_output(network, created, false));

    std::shared_ptr<Node> connected2;
    if (connected == created) {
      connected2 = gate->connect_output(network, created, true);
      assert(connected2 == created);
    }

    return connected;
  }

  std::shared_ptr<const Node> Node_Passthrough_Gated::get_gate() const {
    return m_gate;
  }

  std::shared_ptr<Node> Node_Passthrough_Gated::get_gate() {
    return m_gate;
  }

  bool Node_Passthrough_Gated::operator==(const Node &rhs) const {
    if (auto rhs_passthrough_gated = dynamic_cast<const Node_Passthrough_Gated *>(&rhs)) {
      return get_input() == rhs_passthrough_gated->get_input() && get_gate() == rhs_passthrough_gated->get_gate();
    }

    return false;
  }

}
