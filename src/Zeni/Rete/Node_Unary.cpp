#include "Zeni/Rete/Node_Unary.hpp"

#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Raven_Connect_Output.hpp"
#include "Zeni/Rete/Raven_Disconnect_Output.hpp"
#include "Zeni/Rete/Raven_Input_Disable.hpp"
#include "Zeni/Rete/Raven_Input_Enable.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Unary::Unlocked_Node_Unary_Data::Unlocked_Node_Unary_Data(const std::shared_ptr<Node> input, const bool enabled)
    : m_input(input), m_input_enabled(enabled ? 1 : 0)
  {
  }

  Node_Unary::Locked_Node_Unary_Data_Const::Locked_Node_Unary_Data_Const(const Node_Unary * node, const Locked_Node_Data_Const &)
    : m_data(node->m_unlocked_node_unary_data)
  {
  }

  std::shared_ptr<const Node> Node_Unary::Locked_Node_Unary_Data_Const::get_input() const {
    return m_data->m_input;
  }

  int64_t Node_Unary::Locked_Node_Unary_Data_Const::get_input_enabled() const {
    return m_data->m_input_enabled;
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

  std::shared_ptr<Node> & Node_Unary::Locked_Node_Unary_Data::modify_input() {
    return m_data->m_input;
  }

  int64_t & Node_Unary::Locked_Node_Unary_Data::modify_input_enabled() {
    return m_data->m_input_enabled;
  }

  Tokens & Node_Unary::Locked_Node_Unary_Data::modify_input_tokens() {
    return m_data->m_input_tokens;
  }

  Tokens & Node_Unary::Locked_Node_Unary_Data::modify_input_antitokens() {
    return m_data->m_input_antitokens;
  }

  Node_Unary::Node_Unary(const int64_t height, const int64_t size, const int64_t token_size, const std::shared_ptr<Node> input, const bool enabled)
    : Node(height, size, token_size, true),
    m_unlocked_node_unary_data(std::make_shared<Unlocked_Node_Unary_Data>(input, enabled))
  {
  }

  void Node_Unary::send_connect_to_parents(const std::shared_ptr<Network> network, const Locked_Node_Data &locked_node_data) {
    Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

    locked_node_unary_data.modify_input()->increment_output_count();
    network->get_Job_Queue()->give_one(std::make_shared<Raven_Connect_Output>(locked_node_unary_data.modify_input(), network, shared_from_this()));
  }

  void Node_Unary::send_disconnect_from_parents(const std::shared_ptr<Network> network, const Locked_Node_Data &locked_node_data) {
    Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

    network->get_Job_Queue()->give_one(std::make_shared<Raven_Disconnect_Output>(locked_node_unary_data.modify_input(), network, shared_from_this()));
  }

  ZENI_RETE_LINKAGE std::shared_ptr<Node> Node_Unary::get_input() {
    Locked_Node_Data locked_node_data(this);
    Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

    return locked_node_unary_data.modify_input();
  }

  void Node_Unary::receive(const Raven_Input_Disable &raven) {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::Job>> jobs;

    {
      Locked_Node_Data locked_node_data(this);
      Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

      if (--locked_node_unary_data.modify_input_enabled() == 0) {
        jobs.reserve(locked_node_data.get_outputs().size());
        for (auto &output : locked_node_data.get_outputs())
          jobs.emplace_back(std::make_shared<Raven_Input_Disable>(output, raven.get_Network(), sft));
      }
    }

    raven.get_Network()->get_Job_Queue()->give_many(std::move(jobs));
  }

  void Node_Unary::receive(const Raven_Input_Enable &raven) {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::Job>> jobs;

    {
      Locked_Node_Data locked_node_data(this);
      Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

      if (++locked_node_unary_data.modify_input_enabled() == 1) {
        jobs.reserve(locked_node_data.get_outputs().size());
        for (auto &output : locked_node_data.get_outputs())
          jobs.emplace_back(std::make_shared<Raven_Input_Enable>(output, raven.get_Network(), sft));
      }
    }

    raven.get_Network()->get_Job_Queue()->give_many(std::move(jobs));
  }

}
