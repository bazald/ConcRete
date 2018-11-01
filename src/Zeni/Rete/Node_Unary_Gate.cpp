#include "Zeni/Rete/Node_Unary_Gate.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Internal/Debug_Counters.hpp"
#include "Zeni/Rete/Internal/Message_Connect_Gate.hpp"
#include "Zeni/Rete/Internal/Message_Connect_Output.hpp"
#include "Zeni/Rete/Internal/Message_Disconnect_Gate.hpp"
#include "Zeni/Rete/Internal/Message_Disconnect_Output.hpp"
#include "Zeni/Rete/Internal/Message_Status_Empty.hpp"
#include "Zeni/Rete/Internal/Message_Status_Nonempty.hpp"
#include "Zeni/Rete/Internal/Token_Alpha.hpp"
#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Action.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Unary_Gate::Node_Unary_Gate(const std::shared_ptr<Node> input)
    : Node(input->get_height(), input->get_size(), 0, hash_combine(std::hash<int>()(3), input->get_hash())),
    m_input(input),
    m_nonempty_token(std::make_shared<Token_Alpha>(std::make_shared<WME>(m_nonempty_token_symbol, m_nonempty_token_symbol, m_nonempty_token_symbol)))
  {
  }

  Node_Unary_Gate::~Node_Unary_Gate()
  {
  }

  void Node_Unary_Gate::send_disconnect_from_parents(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) {
    job_queue->give_one(std::make_shared<Message_Disconnect_Gate>(get_input(), network, shared_from_this()));
  }

  std::shared_ptr<Node_Unary_Gate> Node_Unary_Gate::Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> input) {
    class Friendly_Node_Unary_Gate : public Node_Unary_Gate {
    public:
      Friendly_Node_Unary_Gate(const std::shared_ptr<Node> &input) : Node_Unary_Gate(input) {}
    };

    const auto created = std::shared_ptr<Friendly_Node_Unary_Gate>(new Friendly_Node_Unary_Gate(input));
    const auto connected = std::static_pointer_cast<Node_Unary_Gate>(input->connect_new_or_existing_gate(network, job_queue, created));

    if (connected != created)
      DEBUG_COUNTER_DECREMENT(g_decrement_children_received, 1);

    return connected;
  }

  bool Node_Unary_Gate::receive(const Message_Connect_Gate &message) {
    const bool has_tokens_to_pass = Node::receive(message);
    if (!has_tokens_to_pass)
      return false;

    const auto inputs = std::const_pointer_cast<Node>(message.child)->get_inputs();
    inputs.first->connect_existing_gate(message.network, message.get_Job_Queue(), message.child);
    if (inputs.second)
      inputs.second->connect_existing_gate(message.network, message.get_Job_Queue(), message.child);

    return has_tokens_to_pass;
  }

  bool Node_Unary_Gate::receive(const Message_Connect_Output &message) {
    const bool has_tokens_to_pass = Node::receive(message);
    if (!has_tokens_to_pass)
      return false;

    const auto inputs = std::const_pointer_cast<Node>(message.child)->get_inputs();
    inputs.first->connect_existing_output(message.network, message.get_Job_Queue(), message.child);
    if (inputs.second)
      inputs.second->connect_existing_output(message.network, message.get_Job_Queue(), message.child);

    return true;
  }

  bool Node_Unary_Gate::receive(const Message_Disconnect_Gate &message) {
    const bool has_tokens_to_retract = Node::receive(message);
    if (!has_tokens_to_retract)
      return false;

    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    const auto inputs = std::const_pointer_cast<Node>(message.child)->get_inputs();
    jobs.emplace_back(std::make_shared<Message_Disconnect_Gate>(inputs.first, message.network, message.child));
    if (inputs.second)
      jobs.emplace_back(std::make_shared<Message_Disconnect_Gate>(inputs.second, message.network, message.child));

    message.get_Job_Queue()->give_many(std::move(jobs));

    return true;
  }

  bool Node_Unary_Gate::receive(const Message_Disconnect_Output &message) {
    const bool has_tokens_to_retract = Node::receive(message);
    if (!has_tokens_to_retract)
      return false;

    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    const auto inputs = std::const_pointer_cast<Node>(message.child)->get_inputs();
    jobs.emplace_back(std::make_shared<Message_Disconnect_Output>(inputs.first, message.network, message.child));
    if(inputs.second)
      jobs.emplace_back(std::make_shared<Message_Disconnect_Output>(inputs.second, message.network, message.child));

    message.get_Job_Queue()->give_many(std::move(jobs));

    return true;
  }

  void Node_Unary_Gate::receive(const Message_Status_Empty &message) {
    const auto[result, snapshot, value] = m_node_data.insert<NODE_DATA_SUBTRIE_TOKEN_INPUTS_LEFT>(m_nonempty_token);
    if (result != Input_Token_Trie::Result::First_Insertion)
      return;

    for (auto &output : snapshot.template snapshot<NODE_DATA_SUBTRIE_OUTPUTS>()) {
      const auto inputs = output->get_inputs();
      inputs.first->connect_existing_gate(message.network, message.get_Job_Queue(), message.child);
      if (inputs.second)
        inputs.second->connect_existing_gate(message.network, message.get_Job_Queue(), message.child);
    }
    for (auto &output : snapshot.template snapshot<NODE_DATA_SUBTRIE_GATES>()) {
      const auto inputs = output->get_inputs();
      inputs.first->connect_existing_gate(message.network, message.get_Job_Queue(), message.child);
      if (inputs.second)
        inputs.second->connect_existing_gate(message.network, message.get_Job_Queue(), message.child);
    }
  }

  void Node_Unary_Gate::receive(const Message_Status_Nonempty &message) {
    const auto[result, snapshot, value] = m_node_data.erase<NODE_DATA_SUBTRIE_TOKEN_INPUTS_LEFT>(m_nonempty_token);
    if (result != Input_Token_Trie::Result::Last_Removal)
      return;

    for (auto &output : snapshot.template snapshot<NODE_DATA_SUBTRIE_OUTPUTS>()) {
      const auto inputs = output->get_inputs();
      inputs.first->connect_existing_output(message.network, message.get_Job_Queue(), message.child);
      if(inputs.second)
        inputs.second->connect_existing_output(message.network, message.get_Job_Queue(), message.child);
    }
    for (auto &output : snapshot.template snapshot<NODE_DATA_SUBTRIE_GATES>()) {
      const auto inputs = output->get_inputs();
      inputs.first->connect_existing_output(message.network, message.get_Job_Queue(), message.child);
      if (inputs.second)
        inputs.second->connect_existing_output(message.network, message.get_Job_Queue(), message.child);
    }
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
