#include "Zeni/Rete/Node_Action.hpp"

#include "Zeni/Rete/Internal/Message_Token_Insert.hpp"
#include "Zeni/Rete/Internal/Message_Token_Remove.hpp"
#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Key.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Action::Node_Action(const std::string_view name_, const std::shared_ptr<const Node_Key> node_key, const std::shared_ptr<Node> input, const std::shared_ptr<const Variable_Indices> variables, const Action &action_, const Action &retraction_)
    : Node_Unary(input->get_height() + 1, input->get_size(), input->get_token_size(), Hash_By_Name()(name_), node_key, input),
    m_variables(variables),
    m_name(name_),
    m_action(action_),
    m_retraction(retraction_)
  {
    assert(!m_name.empty());
  }

  Node_Action::Node_Action(const std::string_view name_)
    : Node_Unary(1, 0, 0, size_t(this), nullptr, nullptr),
    m_variables(nullptr),
    m_name(name_),
    m_action(Action()),
    m_retraction(Action())
  {
    assert(!m_name.empty());
  }

  std::shared_ptr<Node_Action> Node_Action::Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::string_view name, const bool user_action, const std::shared_ptr<const Node_Key> node_key, const std::shared_ptr<Node> input, const std::shared_ptr<const Variable_Indices> variables, const Node_Action::Action action, const Node_Action::Action retraction) {
    class Friendly_Node_Action : public Node_Action {
    public:
      Friendly_Node_Action(const std::string_view name_, const std::shared_ptr<const Node_Key> node_key_, const std::shared_ptr<Node> &input, const std::shared_ptr<const Variable_Indices> &variables, const Action &action_, const Action &retraction_) : Node_Action(name_, node_key_, input, variables, action_, retraction_) {}
    };

    const auto action_fun = std::shared_ptr<Friendly_Node_Action>(new Friendly_Node_Action(name, node_key, input, variables, action, retraction));

    network->source_rule(job_queue, action_fun, user_action);

    input->connect_new_or_existing_output(network, job_queue, node_key, action_fun);

    return action_fun;
  }

  Node_Action::~Node_Action()
  {
  }

  void Node_Action::Destroy(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) {
    send_disconnect_from_parents(network, job_queue);
  }

  std::string Node_Action::get_name() const {
    return m_name;
  }

  std::shared_ptr<const Variable_Indices> Node_Action::get_variables() const {
    return m_variables;
  }

  std::pair<Node::Node_Trie::Result, std::shared_ptr<Node>> Node_Action::connect_new_or_existing_output(const std::shared_ptr<Network>, const std::shared_ptr<Concurrency::Job_Queue>, const std::shared_ptr<const Node_Key>, const std::shared_ptr<Node>) {
    abort();
  }

  Node::Node_Trie::Result Node_Action::connect_existing_output(const std::shared_ptr<Network>, const std::shared_ptr<Concurrency::Job_Queue>, const std::shared_ptr<const Node_Key>, const std::shared_ptr<Node>) {
    abort();
  }

  void Node_Action::receive(const Message_Disconnect_Output &) {
    abort();
  }

  void Node_Action::receive(const Message_Token_Insert &message) {
    const auto[result, snapshot, value] = m_token_trie.insert(message.token);

    if (result == Token_Trie::Result::First_Insertion)
      m_action(*this, *value);
  }

  void Node_Action::receive(const Message_Token_Remove &message) {
    const auto[result, snapshot, value] = m_token_trie.erase(message.token);

    if (result == Token_Trie::Result::Last_Removal)
      m_retraction(*this, *value);
  }

  bool Node_Action::operator==(const Node &rhs) const {
    return this == &rhs;
  }

}
