#include "Zeni/Rete/Node_Passthrough_Gated.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Internal/Debug_Counters.hpp"
#include "Zeni/Rete/Message_Decrement_Output_Count.hpp"
#include "Zeni/Rete/Message_Connect_Output.hpp"
#include "Zeni/Rete/Message_Disconnect_Output.hpp"
#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Unary_Gate.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Passthrough_Gated::Node_Passthrough_Gated(const std::shared_ptr<Node> input, const std::shared_ptr<Node> gate)
    : Node_Passthrough(input),
    m_gate(gate)
  {
  }

  void Node_Passthrough_Gated::send_disconnect_from_parents(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    jobs.emplace_back(std::make_shared<Message_Decrement_Output_Count>(get_input(), network, sft));
    jobs.emplace_back(std::make_shared<Message_Disconnect_Output>(m_gate, network, sft, true));

    job_queue->give_many(std::move(jobs));
  }

  Node_Passthrough_Gated::~Node_Passthrough_Gated() {
  }

  std::shared_ptr<Node_Passthrough_Gated> Node_Passthrough_Gated::Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> input, const std::shared_ptr<Node> gate) {
    class Friendly_Node_Passthrough_Gated : public Node_Passthrough_Gated {
    public:
      Friendly_Node_Passthrough_Gated(const std::shared_ptr<Node> &input, const std::shared_ptr<Node> gate) : Node_Passthrough_Gated(input, gate) {}
    };

    const auto created = std::make_shared<Friendly_Node_Passthrough_Gated>(input, gate);
    const auto connected = std::dynamic_pointer_cast<Node_Passthrough_Gated>(gate->connect_output(network, job_queue, created));

    if (connected != created) {
      DEBUG_COUNTER_DECREMENT(g_decrement_outputs_received, 2);
      std::vector<std::shared_ptr<Concurrency::IJob>> jobs;
      jobs.emplace_back(std::make_shared<Message_Decrement_Output_Count>(gate, network, created));
      jobs.emplace_back(std::make_shared<Message_Decrement_Output_Count>(input, network, created));
      job_queue->give_many(std::move(jobs));
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
