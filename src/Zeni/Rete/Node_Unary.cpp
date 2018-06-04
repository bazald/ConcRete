#include "Zeni/Rete/Node_Unary.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Raven_Connect_Output.hpp"
#include "Zeni/Rete/Raven_Disconnect_Output.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Unary::Unlocked_Node_Unary_Data::Unlocked_Node_Unary_Data()
  {
  }

  Node_Unary::Locked_Node_Unary_Data_Const::Locked_Node_Unary_Data_Const(const Node_Unary * node, const Locked_Node_Data_Const &)
    : m_data(node->m_unlocked_node_unary_data)
  {
  }

  const Tokens & Node_Unary::Locked_Node_Unary_Data_Const::get_input_tokens() const {
    return m_data->m_input_tokens;
  }

  const Tokens & Node_Unary::Locked_Node_Unary_Data_Const::get_input_antitokens() const {
    return m_data->m_input_antitokens;
  }

  Node_Unary::Locked_Node_Unary_Data::Locked_Node_Unary_Data(Node_Unary * node, const Locked_Node_Data &data)
    : Locked_Node_Unary_Data_Const(node, data),
    m_data(node->m_unlocked_node_unary_data)
  {
  }

  Tokens & Node_Unary::Locked_Node_Unary_Data::modify_input_tokens() {
    return m_data->m_input_tokens;
  }

  Tokens & Node_Unary::Locked_Node_Unary_Data::modify_input_antitokens() {
    return m_data->m_input_antitokens;
  }

  Node_Unary::Node_Unary(const int64_t height, const int64_t size, const int64_t token_size, const std::shared_ptr<Node> input)
    : Node(height, size, token_size),
    m_unlocked_node_unary_data(std::make_shared<Unlocked_Node_Unary_Data>()),
    m_input(input)
  {
  }

  void Node_Unary::send_disconnect_from_parents(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const Locked_Node_Data &) {
    job_queue->give_one(std::make_shared<Raven_Disconnect_Output>(m_input, network, shared_from_this(), true));
  }

  std::pair<std::shared_ptr<Node>, std::shared_ptr<Node>> Node_Unary::get_inputs() {
    return std::make_pair(m_input, nullptr);
  }

  std::shared_ptr<const Node> Node_Unary::get_input() const {
    return m_input;
  }

  std::shared_ptr<Node> Node_Unary::get_input() {
    return m_input;
  }

}
