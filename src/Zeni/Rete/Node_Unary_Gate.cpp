#include "Zeni/Rete/Node_Unary_Gate.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Internal/Debug_Counters.hpp"
#include "Zeni/Rete/Message_Connect_Gate.hpp"
#include "Zeni/Rete/Message_Connect_Output.hpp"
#include "Zeni/Rete/Message_Decrement_Child_Count.hpp"
#include "Zeni/Rete/Message_Disconnect_Gate.hpp"
#include "Zeni/Rete/Message_Disconnect_Output.hpp"
#include "Zeni/Rete/Message_Status_Empty.hpp"
#include "Zeni/Rete/Message_Status_Nonempty.hpp"
#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Action.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Unary_Gate::Unlocked_Node_Unary_Gate_Data::Unlocked_Node_Unary_Gate_Data()
  {
  }

  Node_Unary_Gate::Locked_Node_Unary_Gate_Data_Const::Locked_Node_Unary_Gate_Data_Const(const Node_Unary_Gate * node, const Locked_Node_Data_Const &)
    : m_data(node->m_unlocked_node_unary_gate_data)
  {
  }

  int64_t Node_Unary_Gate::Locked_Node_Unary_Gate_Data_Const::get_input_tokens() const {
    return m_data->m_input_tokens;
  }

  Node_Unary_Gate::Locked_Node_Unary_Gate_Data::Locked_Node_Unary_Gate_Data(Node_Unary_Gate * node, const Locked_Node_Data &data)
    : Locked_Node_Unary_Gate_Data_Const(node, data),
    m_data(node->m_unlocked_node_unary_gate_data)
  {
  }

  int64_t & Node_Unary_Gate::Locked_Node_Unary_Gate_Data::modify_input_tokens() {
    return m_data->m_input_tokens;
  }

  Node_Unary_Gate::Node_Unary_Gate(const std::shared_ptr<Node> input)
    : Node(input->get_height(), input->get_size(), 0, hash_combine(std::hash<int>()(3), input->get_hash())),
    m_unlocked_node_unary_gate_data(std::make_shared<Unlocked_Node_Unary_Gate_Data>()),
    m_input(input)
  {
  }

  Node_Unary_Gate::~Node_Unary_Gate()
  {
  }

  void Node_Unary_Gate::send_disconnect_from_parents(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) {
    job_queue->give_one(std::make_shared<Message_Disconnect_Gate>(get_input(), network, shared_from_this(), true));
  }

  std::shared_ptr<Node_Unary_Gate> Node_Unary_Gate::Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> input) {
    class Friendly_Node_Unary_Gate : public Node_Unary_Gate {
    public:
      Friendly_Node_Unary_Gate(const std::shared_ptr<Node> &input) : Node_Unary_Gate(input) {}
    };

    const auto created = std::make_shared<Friendly_Node_Unary_Gate>(input);
    const auto connected = std::static_pointer_cast<Node_Unary_Gate>(input->connect_gate(network, job_queue, created));

    if (connected != created) {
      DEBUG_COUNTER_DECREMENT(g_decrement_children_received, 1);
      job_queue->give_one(std::make_shared<Message_Decrement_Child_Count>(input, network));
    }

    return connected;
  }

  void Node_Unary_Gate::receive(const Message_Connect_Gate &message) {
    Locked_Node_Data locked_node_data(this);

    const bool first_insertion = Node::receive(message, locked_node_data);
    if (!first_insertion)
      return;

    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    {
      Locked_Node_Unary_Gate_Data locked_node_unary_gate_data(this, locked_node_data);

      const int64_t tokens_input = locked_node_unary_gate_data.get_input_tokens();

      if (tokens_input > 0) {
        const auto inputs = std::const_pointer_cast<Node>(message.child)->get_inputs();
        jobs.emplace_back(std::make_shared<Message_Connect_Gate>(inputs.first, message.network, message.child));
        if (inputs.second)
          jobs.emplace_back(std::make_shared<Message_Connect_Gate>(inputs.second, message.network, message.child));
      }
    }

    message.get_Job_Queue()->give_many(std::move(jobs));
  }

  void Node_Unary_Gate::receive(const Message_Connect_Output &message) {
    Locked_Node_Data locked_node_data(this);

    const bool first_insertion = Node::receive(message, locked_node_data);
    if (!first_insertion)
      return;

    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    {
      Locked_Node_Unary_Gate_Data locked_node_unary_gate_data(this, locked_node_data);

      const int64_t tokens_input = locked_node_unary_gate_data.get_input_tokens();

      if (tokens_input > 0) {
        const auto inputs = std::const_pointer_cast<Node>(message.child)->get_inputs();
        jobs.emplace_back(std::make_shared<Message_Connect_Output>(inputs.first, message.network, message.child));
        if (inputs.second)
          jobs.emplace_back(std::make_shared<Message_Connect_Output>(inputs.second, message.network, message.child));
      }
    }

    message.get_Job_Queue()->give_many(std::move(jobs));
  }

  void Node_Unary_Gate::receive(const Message_Disconnect_Gate &message) {
    Locked_Node_Data locked_node_data(this);

    const bool erased_last = Node::receive(message, locked_node_data);
    if (!erased_last)
      return;

    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    {
      Locked_Node_Unary_Gate_Data locked_node_unary_gate_data(this, locked_node_data);

      const int64_t tokens_input = locked_node_unary_gate_data.get_input_tokens();

      if (tokens_input > 0) {
        const auto inputs = std::const_pointer_cast<Node>(message.child)->get_inputs();
        jobs.emplace_back(std::make_shared<Message_Disconnect_Gate>(inputs.first, message.network, message.child, false));
        if (inputs.second)
          jobs.emplace_back(std::make_shared<Message_Disconnect_Gate>(inputs.second, message.network, message.child, false));
      }
    }

    message.get_Job_Queue()->give_many(std::move(jobs));
  }

  void Node_Unary_Gate::receive(const Message_Disconnect_Output &message) {
    Locked_Node_Data locked_node_data(this);

    const bool erased_last = Node::receive(message, locked_node_data);
    if (!erased_last)
      return;

    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    {
      Locked_Node_Unary_Gate_Data locked_node_unary_gate_data(this, locked_node_data);

      const int64_t tokens_input = locked_node_unary_gate_data.get_input_tokens();

      if (tokens_input > 0) {
        const auto inputs = std::const_pointer_cast<Node>(message.child)->get_inputs();
        jobs.emplace_back(std::make_shared<Message_Disconnect_Output>(inputs.first, message.network, message.child, false));
        if(inputs.second)
          jobs.emplace_back(std::make_shared<Message_Disconnect_Output>(inputs.second, message.network, message.child, false));
      }
    }

    message.get_Job_Queue()->give_many(std::move(jobs));
  }

  void Node_Unary_Gate::receive(const Message_Status_Empty &message) {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    {
      Locked_Node_Data locked_node_data(this);
      Locked_Node_Unary_Gate_Data locked_node_unary_gate_data(this, locked_node_data);

      const Outputs &gates = locked_node_data.get_gates();
      const Outputs &outputs = locked_node_data.get_outputs();
      int64_t &tokens_input = locked_node_unary_gate_data.modify_input_tokens();

      if (--tokens_input != 0)
        return;

      size_t num_jobs = 0;
      for (auto &output : outputs)
        num_jobs += output->get_inputs().second ? 2 : 1;
      for (auto &output : gates)
        num_jobs += output->get_inputs().second ? 2 : 1;

      jobs.reserve(num_jobs);
      for (auto &output : outputs) {
        const auto inputs = output->get_inputs();
        jobs.emplace_back(std::make_shared<Message_Disconnect_Output>(inputs.first, message.network, output, false));
        if (inputs.second)
          jobs.emplace_back(std::make_shared<Message_Disconnect_Output>(inputs.second, message.network, output, false));
      }
      for (auto &output : gates) {
        const auto inputs = output->get_inputs();
        jobs.emplace_back(std::make_shared<Message_Disconnect_Gate>(inputs.first, message.network, output, false));
        if (inputs.second)
          jobs.emplace_back(std::make_shared<Message_Disconnect_Gate>(inputs.second, message.network, output, false));
      }
    }

    message.get_Job_Queue()->give_many(std::move(jobs));
  }

  void Node_Unary_Gate::receive(const Message_Status_Nonempty &message) {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    {
      Locked_Node_Data locked_node_data(this);
      Locked_Node_Unary_Gate_Data locked_node_unary_gate_data(this, locked_node_data);

      const Outputs &gates = locked_node_data.get_gates();
      const Outputs &outputs = locked_node_data.get_outputs();
      int64_t &tokens_input = locked_node_unary_gate_data.modify_input_tokens();

      if (++tokens_input != 1)
        return;

      size_t num_jobs = 0;
      for (auto &output : outputs)
        num_jobs += output->get_inputs().second ? 2 : 1;
      for (auto &output : gates)
        num_jobs += output->get_inputs().second ? 2 : 1;

      jobs.reserve(num_jobs);
      for (auto &output : outputs) {
        const auto inputs = output->get_inputs();
        jobs.emplace_back(std::make_shared<Message_Connect_Output>(inputs.first, message.network, output));
        if(inputs.second)
          jobs.emplace_back(std::make_shared<Message_Connect_Output>(inputs.second, message.network, output));
      }
      for (auto &output : gates) {
        const auto inputs = output->get_inputs();
        jobs.emplace_back(std::make_shared<Message_Connect_Gate>(inputs.first, message.network, output));
        if (inputs.second)
          jobs.emplace_back(std::make_shared<Message_Connect_Gate>(inputs.second, message.network, output));
      }
    }

    message.get_Job_Queue()->give_many(std::move(jobs));
  }

  void Node_Unary_Gate::receive(const Message_Token_Insert &) {
    abort();
  }

  void Node_Unary_Gate::receive(const Message_Token_Remove &) {
    abort();
  }

  bool Node_Unary_Gate::operator==(const Node &rhs) const {
    return dynamic_cast<const Node_Unary_Gate *>(&rhs);
  }

  std::pair<std::shared_ptr<Node>, std::shared_ptr<Node>> Node_Unary_Gate::get_inputs() {
    return std::make_pair(m_input, nullptr);
  }

  std::shared_ptr<const Node> Node_Unary_Gate::get_input() const {
    return m_input;
  }

  std::shared_ptr<Node> Node_Unary_Gate::get_input() {
    return m_input;
  }

}
