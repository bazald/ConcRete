#include "Zeni/Rete/Node_Action.hpp"

#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Raven_Token_Insert.hpp"
#include "Zeni/Rete/Raven_Token_Remove.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Action::Node_Action(const std::string_view name_, const std::shared_ptr<Pseudonode> input, const std::shared_ptr<const Variable_Indices> variables,
    const Action &action_,
    const Action &retraction_)
    : Node_Unary(std::dynamic_pointer_cast<Node>(input) ? std::dynamic_pointer_cast<Node>(input)->get_height() + 1 : 1,
      std::dynamic_pointer_cast<Node>(input) ? std::dynamic_pointer_cast<Node>(input)->get_size() : 0,
      std::dynamic_pointer_cast<Node>(input) ? std::dynamic_pointer_cast<Node>(input)->get_token_size() : 0,
      input),
    m_variables(variables),
    m_name(name_),
    m_action(action_),
    m_retraction(retraction_)
  {
    assert(!m_name.empty());
  }

  std::shared_ptr<Node_Action> Node_Action::Create(const std::shared_ptr<Network> network, const std::string_view name, const bool user_action, const std::shared_ptr<Pseudonode> input, const std::shared_ptr<const Variable_Indices> variables, const Node_Action::Action action, const Node_Action::Action retraction) {
    class Friendly_Node_Action : public Node_Action {
    public:
      Friendly_Node_Action(const std::string_view name_, const std::shared_ptr<Pseudonode> &input, const std::shared_ptr<const Variable_Indices> &variables,
        const Action &action_,
        const Action &retraction_) : Node_Action(name_, input, variables, action_, retraction_) {}
    };

    const auto action_fun = std::make_shared<Friendly_Node_Action>(name, input, variables, action, retraction);

    network->source_rule(action_fun, user_action);

    input->connect_output(network, action_fun);

    return action_fun;
  }

  Node_Action::~Node_Action() {
    //for (auto &token : get_input_tokens())
    //  m_retraction(*this, *token);
  }

  std::string Node_Action::get_name() const {
    return m_name;
  }

  std::shared_ptr<const Variable_Indices> Node_Action::get_variables() const {
    return m_variables;
  }

  bool Node_Action::receive(const Raven_Token_Insert &raven) {
    if (Node_Unary::receive(raven)) {
      m_action(*this, *raven.get_Token());
      return true;
    }
    else
      return false;
  }

  bool Node_Action::receive(const Raven_Token_Remove &raven) {
    if (Node_Unary::receive(raven)) {
      m_retraction(*this, *raven.get_Token());
      return true;
    }
    else
      return false;
  }

  bool Node_Action::operator==(const Node &) const {
    return false;
  }

}
