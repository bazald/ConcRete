#include "Zeni/Rete/Node_Unary_Gate.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Raven_Connect_Gate.hpp"
#include "Zeni/Rete/Raven_Connect_Output.hpp"
#include "Zeni/Rete/Raven_Decrement_Output_Count.hpp"
#include "Zeni/Rete/Raven_Disconnect_Gate.hpp"
#include "Zeni/Rete/Raven_Disconnect_Output.hpp"
#include "Zeni/Rete/Raven_Status_Empty.hpp"
#include "Zeni/Rete/Raven_Status_Nonempty.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Unary_Gate::Node_Unary_Gate(const std::shared_ptr<Node> input)
    : Node_Unary(input->get_height(), input->get_size(), 0, input)
  {
  }

  Node_Unary_Gate::~Node_Unary_Gate()
  {
  }

  void Node_Unary_Gate::send_disconnect_from_parents(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) {
    job_queue->give_one(std::make_shared<Raven_Disconnect_Gate>(get_input(), network, shared_from_this(), true));
  }

  std::shared_ptr<Node_Unary_Gate> Node_Unary_Gate::Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> input) {
    class Friendly_Node_Unary_Gate : public Node_Unary_Gate {
    public:
      Friendly_Node_Unary_Gate(const std::shared_ptr<Node> &input) : Node_Unary_Gate(input) {}
    };

    const auto created = std::make_shared<Friendly_Node_Unary_Gate>(input);
    const auto connected = std::static_pointer_cast<Node_Unary_Gate>(input->connect_gate(network, job_queue, created));

    if (connected != created) {
      Zeni::Rete::Counters::g_decrement_outputs_received.fetch_sub(1, std::memory_order_acquire);
      job_queue->give_one(std::make_shared<Raven_Decrement_Output_Count>(input, network, created));
    }

    return connected;
  }

  void Node_Unary_Gate::receive(const Raven_Connect_Gate &raven) {
    Locked_Node_Data locked_node_data(this);

    const bool first_insertion = Node::receive(raven, locked_node_data);
    if (!first_insertion)
      return;

    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    {
      Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

      const Tokens_Input &tokens_input = locked_node_unary_data.get_input_tokens();

      if (!tokens_input.positive.empty()) {
        const auto inputs = std::const_pointer_cast<Node>(raven.get_sender())->get_inputs();
        jobs.emplace_back(std::make_shared<Raven_Connect_Gate>(inputs.first, raven.get_Network(), raven.get_sender()));
        if (inputs.second)
          jobs.emplace_back(std::make_shared<Raven_Connect_Gate>(inputs.second, raven.get_Network(), raven.get_sender()));
      }
    }

    raven.get_Job_Queue()->give_many(std::move(jobs));
  }

  void Node_Unary_Gate::receive(const Raven_Connect_Output &raven) {
    Locked_Node_Data locked_node_data(this);

    const bool first_insertion = Node::receive(raven, locked_node_data);
    if (!first_insertion)
      return;

    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    {
      Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

      const Tokens_Input &tokens_input = locked_node_unary_data.get_input_tokens();

      if (!tokens_input.positive.empty()) {
        const auto inputs = std::const_pointer_cast<Node>(raven.get_sender())->get_inputs();
        jobs.emplace_back(std::make_shared<Raven_Connect_Output>(inputs.first, raven.get_Network(), raven.get_sender()));
        if (inputs.second)
          jobs.emplace_back(std::make_shared<Raven_Connect_Output>(inputs.second, raven.get_Network(), raven.get_sender()));
      }
    }

    raven.get_Job_Queue()->give_many(std::move(jobs));
  }

  void Node_Unary_Gate::receive(const Raven_Disconnect_Gate &raven) {
    Locked_Node_Data locked_node_data(this);

    const bool erased_last = Node::receive(raven, locked_node_data);
    if (!erased_last)
      return;

    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    {
      Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

      const Tokens_Input &tokens_input = locked_node_unary_data.get_input_tokens();

      if (!tokens_input.positive.empty()) {
        const auto inputs = std::const_pointer_cast<Node>(raven.get_sender())->get_inputs();
        jobs.emplace_back(std::make_shared<Raven_Disconnect_Gate>(inputs.first, raven.get_Network(), raven.get_sender(), false));
        if (inputs.second)
          jobs.emplace_back(std::make_shared<Raven_Disconnect_Gate>(inputs.second, raven.get_Network(), raven.get_sender(), false));
      }
    }

    raven.get_Job_Queue()->give_many(std::move(jobs));
  }

  void Node_Unary_Gate::receive(const Raven_Disconnect_Output &raven) {
    Locked_Node_Data locked_node_data(this);

    const bool erased_last = Node::receive(raven, locked_node_data);
    if (!erased_last)
      return;

    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    {
      Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

      const Tokens_Input &tokens_input = locked_node_unary_data.get_input_tokens();

      if (!tokens_input.positive.empty()) {
        const auto inputs = std::const_pointer_cast<Node>(raven.get_sender())->get_inputs();
        jobs.emplace_back(std::make_shared<Raven_Disconnect_Output>(inputs.first, raven.get_Network(), raven.get_sender(), false));
        if(inputs.second)
          jobs.emplace_back(std::make_shared<Raven_Disconnect_Output>(inputs.second, raven.get_Network(), raven.get_sender(), false));
      }
    }

    raven.get_Job_Queue()->give_many(std::move(jobs));
  }

  void Node_Unary_Gate::receive(const Raven_Status_Empty &raven) {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    {
      Locked_Node_Data locked_node_data(this);
      Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

      const Outputs &gates = locked_node_data.get_gates();
      const Outputs &outputs = locked_node_data.get_outputs();
      Tokens_Input &tokens_input = locked_node_unary_data.modify_input_tokens();

      if (tokens_input.positive.empty()) {
        tokens_input.negative.emplace(std::shared_ptr<Token>());
        return;
      }

      tokens_input.positive.erase(tokens_input.positive.begin());
      if (!tokens_input.positive.empty())
        return;

      size_t num_jobs = 0;
      for (auto &output : outputs.positive)
        num_jobs += output->get_inputs().second ? 2 : 1;
      for (auto &output : gates.positive)
        num_jobs += output->get_inputs().second ? 2 : 1;

      jobs.reserve(num_jobs);
      for (auto &output : outputs.positive) {
        const auto inputs = output->get_inputs();
        jobs.emplace_back(std::make_shared<Raven_Disconnect_Output>(inputs.first, raven.get_Network(), output, false));
        if (inputs.second)
          jobs.emplace_back(std::make_shared<Raven_Disconnect_Output>(inputs.second, raven.get_Network(), output, false));
      }
      for (auto &output : gates.positive) {
        const auto inputs = output->get_inputs();
        jobs.emplace_back(std::make_shared<Raven_Disconnect_Gate>(inputs.first, raven.get_Network(), output, false));
        if (inputs.second)
          jobs.emplace_back(std::make_shared<Raven_Disconnect_Gate>(inputs.second, raven.get_Network(), output, false));
      }
    }

    raven.get_Job_Queue()->give_many(std::move(jobs));
  }

  void Node_Unary_Gate::receive(const Raven_Status_Nonempty &raven) {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    {
      Locked_Node_Data locked_node_data(this);
      Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

      const Outputs &gates = locked_node_data.get_gates();
      const Outputs &outputs = locked_node_data.get_outputs();
      Tokens_Input &tokens_input = locked_node_unary_data.modify_input_tokens();

      if (!tokens_input.negative.empty()) {
        tokens_input.negative.erase(tokens_input.negative.begin());
        return;
      }

      const bool first_insertion = tokens_input.positive.empty();
      tokens_input.positive.emplace(std::shared_ptr<Token>());
      if (!first_insertion)
        return;

      size_t num_jobs = 0;
      for (auto &output : outputs.positive)
        num_jobs += output->get_inputs().second ? 2 : 1;
      for (auto &output : gates.positive)
        num_jobs += output->get_inputs().second ? 2 : 1;

      jobs.reserve(num_jobs);
      for (auto &output : outputs.positive) {
        const auto inputs = output->get_inputs();
        jobs.emplace_back(std::make_shared<Raven_Connect_Output>(inputs.first, raven.get_Network(), output));
        if(inputs.second)
          jobs.emplace_back(std::make_shared<Raven_Connect_Output>(inputs.second, raven.get_Network(), output));
      }
      for (auto &output : gates.positive) {
        const auto inputs = output->get_inputs();
        jobs.emplace_back(std::make_shared<Raven_Connect_Gate>(inputs.first, raven.get_Network(), output));
        if (inputs.second)
          jobs.emplace_back(std::make_shared<Raven_Connect_Gate>(inputs.second, raven.get_Network(), output));
      }
    }

    raven.get_Job_Queue()->give_many(std::move(jobs));
  }

  void Node_Unary_Gate::receive(const Raven_Token_Insert &) {
    abort();
  }

  void Node_Unary_Gate::receive(const Raven_Token_Remove &) {
    abort();
  }

  bool Node_Unary_Gate::operator==(const Node &rhs) const {
    return dynamic_cast<const Node_Unary_Gate *>(&rhs);
  }

}
