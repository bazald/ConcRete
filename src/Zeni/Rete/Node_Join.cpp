#include "Zeni/Rete/Node_Join.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Internal/Debug_Counters.hpp"
#include "Zeni/Rete/Internal/Message_Token_Insert.hpp"
#include "Zeni/Rete/Internal/Message_Token_Remove.hpp"
#include "Zeni/Rete/Internal/Token_Beta.hpp"
#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Action.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Join::Node_Join(const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const Variable_Bindings &variable_bindings)
    : Node_Binary(std::max(input_left->get_height(), input_right->get_height()) + 1, input_left->get_size() + input_right->get_size() + 1, input_left->get_token_size() + input_right->get_token_size(), hash_combine(hash_combine(std::hash<int>()(3), input_left->get_hash()), input_right->get_hash()), input_left, input_right),
    m_variable_bindings(variable_bindings)
  {
  }

  Node_Join::Node_Join(const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, Variable_Bindings &&variable_bindings)
    : Node_Binary(std::max(input_left->get_height(), input_right->get_height()) + 1, input_left->get_size() + input_right->get_size() + 1, input_left->get_token_size() + input_right->get_token_size(), hash_combine(hash_combine(std::hash<int>()(3), input_left->get_hash()), input_right->get_hash()), input_left, input_right),
    m_variable_bindings(std::move(variable_bindings))
  {
  }

  Node_Join::Node_Join(const size_t hash, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const Variable_Bindings &variable_bindings)
    : Node_Binary(std::max(input_left->get_height(), input_right->get_height()) + 1, input_left->get_size() + input_right->get_size() + 1, input_left->get_token_size() + input_right->get_token_size(), hash, input_left, input_right),
    m_variable_bindings(variable_bindings)
  {
  }

  Node_Join::Node_Join(const size_t hash, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, Variable_Bindings &&variable_bindings)
    : Node_Binary(std::max(input_left->get_height(), input_right->get_height()) + 1, input_left->get_size() + input_right->get_size() + 1, input_left->get_token_size() + input_right->get_token_size(), hash, input_left, input_right),
    m_variable_bindings(std::move(variable_bindings))
  {
  }

  Node_Join::~Node_Join()
  {
  }

  std::shared_ptr<Node_Join> Node_Join::Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const Variable_Bindings &variable_bindings) {
    class Friendly_Node_Join : public Node_Join {
    public:
      Friendly_Node_Join(const std::shared_ptr<Node> &input_left, const std::shared_ptr<Node> &input_right, const Variable_Bindings &variable_bindings) : Node_Join(input_left, input_right, variable_bindings) {}
    };

    const auto created = std::shared_ptr<Friendly_Node_Join>(new Friendly_Node_Join(input_left, input_right, variable_bindings));

    return connect_created(network, job_queue, input_left, input_right, created);
  }

  std::shared_ptr<Node_Join> Node_Join::Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, Variable_Bindings &&variable_bindings) {
    class Friendly_Node_Join : public Node_Join {
    public:
      Friendly_Node_Join(const std::shared_ptr<Node> &input_left, const std::shared_ptr<Node> &input_right, Variable_Bindings &&variable_bindings) : Node_Join(input_left, input_right, std::move(variable_bindings)) {}
    };

    const auto created = std::shared_ptr<Friendly_Node_Join>(new Friendly_Node_Join(input_left, input_right, std::move(variable_bindings)));

    return connect_created(network, job_queue, input_left, input_right, created);
  }

  std::shared_ptr<Node_Join> Node_Join::connect_created(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const std::shared_ptr<Node_Join> created) {
    const auto connected = std::static_pointer_cast<Node_Join>(input_left->connect_new_or_existing_output(network, job_queue, created));

    if (input_left != input_right)
      input_right->connect_existing_output(network, job_queue, connected);

    if (connected != created) {
      input_left->send_disconnect_from_parents(network, job_queue);
      if (input_left != input_right) {
        input_right->send_disconnect_from_parents(network, job_queue);
        DEBUG_COUNTER_DECREMENT(g_decrement_children_received, 2);
      }
      else {
        DEBUG_COUNTER_DECREMENT(g_decrement_children_received, 1);
      }
    }

    return connected;
  }

  void Node_Join::receive(const Message_Token_Insert &message) {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    if (message.parent == get_input_left()) {
      const auto[result, snapshot, value] = m_node_data.insert<NODE_DATA_SUBTRIE_TOKEN_INPUTS_LEFT>(message.token);
      if (result != Input_Token_Trie::Result::First_Insertion)
        return;

      if (get_input_left() == get_input_right()) {
        if (!test_variable_bindings(message.token, message.token))
          return;

        const auto output_token = std::make_shared<Token_Beta>(message.token, message.token);
        const auto[oresult, osnapshot, ovalue] = m_node_data.insert<NODE_DATA_SUBTRIE_TOKEN_OUTPUTS>(output_token);
        if (oresult != Output_Token_Trie::Result::First_Insertion)
          return;

        for (auto &output : osnapshot.snapshot<NODE_DATA_SUBTRIE_OUTPUTS>())
          jobs.emplace_back(std::make_shared<Message_Token_Insert>(output, message.network, sft, output_token));
      }
      else {
        for (auto token_right : snapshot.snapshot<NODE_DATA_SUBTRIE_TOKEN_INPUTS_RIGHT>()) {
          if (!test_variable_bindings(message.token, token_right))
            continue;

          const auto output_token = std::make_shared<Token_Beta>(message.token, token_right);
          const auto[oresult, osnapshot, ovalue] = m_node_data.insert<NODE_DATA_SUBTRIE_TOKEN_OUTPUTS>(output_token);
          if (oresult != Output_Token_Trie::Result::First_Insertion)
            continue;

          for (auto &output : osnapshot.snapshot<NODE_DATA_SUBTRIE_OUTPUTS>())
            jobs.emplace_back(std::make_shared<Message_Token_Insert>(output, message.network, sft, output_token));
        }
      }
    }
    else {
      assert(message.parent == get_input_right());

      const auto[result, snapshot, value] = m_node_data.insert<NODE_DATA_SUBTRIE_TOKEN_INPUTS_RIGHT>(message.token);
      if (result != Input_Token_Trie::Result::First_Insertion)
        return;

      for (auto token_left : snapshot.snapshot<NODE_DATA_SUBTRIE_TOKEN_INPUTS_LEFT>()) {
        if (!test_variable_bindings(token_left, message.token))
          continue;

        const auto output_token = std::make_shared<Token_Beta>(token_left, message.token);
        const auto[oresult, osnapshot, ovalue] = m_node_data.insert<NODE_DATA_SUBTRIE_TOKEN_OUTPUTS>(output_token);
        if (oresult != Output_Token_Trie::Result::First_Insertion)
          continue;

        for (auto &output : osnapshot.snapshot<NODE_DATA_SUBTRIE_OUTPUTS>())
          jobs.emplace_back(std::make_shared<Message_Token_Insert>(output, message.network, sft, output_token));
      }
    }

    message.get_Job_Queue()->give_many(std::move(jobs));
  }

  void Node_Join::receive(const Message_Token_Remove &message) {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    if (message.parent == get_input_left()) {
      const auto[result, snapshot, value] = m_node_data.erase<NODE_DATA_SUBTRIE_TOKEN_INPUTS_LEFT>(message.token);
      if (result != Input_Token_Trie::Result::Last_Removal)
        return;

      if (get_input_left() == get_input_right()) {
        if (!test_variable_bindings(message.token, message.token))
          return;

        const auto output_token = std::make_shared<Token_Beta>(value, value);
        const auto[oresult, osnapshot, ovalue] = m_node_data.erase<NODE_DATA_SUBTRIE_TOKEN_OUTPUTS>(output_token);
        if (oresult != Output_Token_Trie::Result::Last_Removal)
          return;

        for (auto &output : osnapshot.snapshot<NODE_DATA_SUBTRIE_OUTPUTS>())
          jobs.emplace_back(std::make_shared<Message_Token_Remove>(output, message.network, sft, ovalue));
      }
      else {
        for (auto token_right : snapshot.snapshot<NODE_DATA_SUBTRIE_TOKEN_INPUTS_RIGHT>()) {
          if (!test_variable_bindings(message.token, token_right))
            continue;

          const auto output_token = std::make_shared<Token_Beta>(value, token_right);
          const auto[oresult, osnapshot, ovalue] = m_node_data.erase<NODE_DATA_SUBTRIE_TOKEN_OUTPUTS>(output_token);
          if (oresult != Output_Token_Trie::Result::Last_Removal)
            continue;

          for (auto &output : osnapshot.snapshot<NODE_DATA_SUBTRIE_OUTPUTS>())
            jobs.emplace_back(std::make_shared<Message_Token_Remove>(output, message.network, sft, ovalue));
        }
      }
    }
    else {
      assert(message.parent == get_input_right());

      const auto[result, snapshot, value] = m_node_data.erase<NODE_DATA_SUBTRIE_TOKEN_INPUTS_RIGHT>(message.token);
      if (result != Input_Token_Trie::Result::Last_Removal)
        return;

      for (auto token_left : snapshot.snapshot<NODE_DATA_SUBTRIE_TOKEN_INPUTS_LEFT>()) {
        if (!test_variable_bindings(token_left, message.token))
          continue;

        const auto output_token = std::make_shared<Token_Beta>(token_left, value);
        const auto[oresult, osnapshot, ovalue] = m_node_data.erase<NODE_DATA_SUBTRIE_TOKEN_OUTPUTS>(output_token);
        if (oresult != Output_Token_Trie::Result::Last_Removal)
          return;

        for (auto &output : osnapshot.snapshot<NODE_DATA_SUBTRIE_OUTPUTS>())
          jobs.emplace_back(std::make_shared<Message_Token_Remove>(output, message.network, sft, ovalue));
      }
    }

    message.get_Job_Queue()->give_many(std::move(jobs));
  }

  bool Node_Join::test_variable_bindings(const std::shared_ptr<const Token> token_left, const std::shared_ptr<const Token> token_right) const {
    for (auto variable_binding : m_variable_bindings) {
      if (*(*token_left)[variable_binding.first] != *(*token_right)[variable_binding.second])
        return false;
    }
    return true;
  }

  bool Node_Join::operator==(const Node &rhs) const {
    //return this == &rhs;

    if (auto rhs_join = dynamic_cast<const Node_Join *>(&rhs))
      return get_input_left() == rhs_join->get_input_left() && get_input_right() == rhs_join->get_input_right();

    return false;
  }

}
