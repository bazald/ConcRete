#include "Zeni/Rete/Node_Action.hpp"

#include "Zeni/Rete/Message_Token_Insert.hpp"
#include "Zeni/Rete/Message_Token_Remove.hpp"
#include "Zeni/Rete/Network.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Action::Node_Action(const std::string_view name_, const std::shared_ptr<Node> input, const std::shared_ptr<const Variable_Indices> variables,
    const Action &action_,
    const Action &retraction_)
    : Node_Unary(input->get_height() + 1, input->get_size(), input->get_token_size(), size_t(this), input),
    m_variables(variables),
    m_name(name_),
    m_action(action_),
    m_retraction(retraction_)
  {
    assert(!m_name.empty());
  }

  std::shared_ptr<Node_Action> Node_Action::Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::string_view name, const bool user_action, const std::shared_ptr<Node> input, const std::shared_ptr<const Variable_Indices> variables, const Node_Action::Action action, const Node_Action::Action retraction) {
    class Friendly_Node_Action : public Node_Action {
    public:
      Friendly_Node_Action(const std::string_view name_, const std::shared_ptr<Node> &input, const std::shared_ptr<const Variable_Indices> &variables,
        const Action &action_,
        const Action &retraction_) : Node_Action(name_, input, variables, action_, retraction_) {}
    };

    const auto action_fun = std::make_shared<Friendly_Node_Action>(name, input, variables, action, retraction);

    network->source_rule(job_queue, action_fun, user_action);

    input->connect_output(network, job_queue, action_fun);

    return action_fun;
  }

  Node_Action::~Node_Action()
  {
  }

  std::string Node_Action::get_name() const {
    return m_name;
  }

  std::shared_ptr<const Variable_Indices> Node_Action::get_variables() const {
    return m_variables;
  }

  void Node_Action::receive(const Message_Status_Empty &) {
    abort();
  }

  void Node_Action::receive(const Message_Status_Nonempty &) {
    abort();
  }

  void Node_Action::receive(const Message_Token_Insert &message) {
    const auto sft = shared_from_this();

    {
      Locked_Node_Data locked_node_data(this);
      Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

      Tokens_Input &tokens_input = locked_node_unary_data.modify_input_tokens();

      auto found = tokens_input.negative.find(message.token);
      if (found != tokens_input.negative.end()) {
        tokens_input.negative.erase(found);
        return;
      }

      tokens_input.positive.emplace(message.token);
    }

    m_action(*this, *message.token);
  }

  void Node_Action::receive(const Message_Token_Remove &message) {
    const auto sft = shared_from_this();

    {
      Locked_Node_Data locked_node_data(this);
      Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

      Tokens_Input &tokens_input = locked_node_unary_data.modify_input_tokens();

      auto found = tokens_input.positive.find(message.token);
      if (found == tokens_input.positive.end()) {
        tokens_input.negative.emplace(message.token);
        return;
      }

      tokens_input.positive.erase(found);
    }

    m_retraction(*this, *message.token);
  }

  bool Node_Action::operator==(const Node &rhs) const {
    return this == &rhs;
  }

}
