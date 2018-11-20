#include "Zeni/Rete/Node_Action.hpp"

#include "Zeni/Rete/Internal/Message_Token_Insert.hpp"
#include "Zeni/Rete/Internal/Message_Token_Remove.hpp"
#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Key.hpp"

#include <cassert>

namespace Zeni::Rete {

  std::shared_ptr<const Node_Action::Null_Action> Node_Action::Null_Action::Create() {
    class Friendly_Null_Action : public Null_Action {
    public:
      Friendly_Null_Action() {}
    };

    static const auto null_action = std::make_shared<Friendly_Null_Action>();

    return null_action;
  }

  Node_Action::Node_Action(const std::string_view name_, const std::shared_ptr<const Node_Key> node_key, const std::shared_ptr<Node> input, const std::shared_ptr<const Variable_Indices> variable_indices, const std::shared_ptr<const Action> action_, const std::shared_ptr<const Action> retraction_)
    : Node_Unary(input->get_height() + 1, input->get_size(), input->get_token_size(), Hash_By_Name()(name_), node_key, input),
    m_variable_indices(variable_indices),
    m_name(name_),
    m_action(action_),
    m_retraction(retraction_)
  {
    assert(!m_name.empty());
  }

  Node_Action::Node_Action(const std::string_view name_)
    : Node_Unary(1, 0, 0, size_t(this), nullptr, nullptr),
    m_variable_indices(nullptr),
    m_name(name_),
    m_action(nullptr),
    m_retraction(nullptr)
  {
    assert(!m_name.empty());
  }

  std::shared_ptr<Node_Action> Node_Action::Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::string_view name, const bool user_action, const std::shared_ptr<const Node_Key> node_key, const std::shared_ptr<Node> input, const std::shared_ptr<const Variable_Indices> variable_indices, const std::shared_ptr<const Action> action, const std::shared_ptr<const Action> retraction) {
    class Friendly_Node_Action : public Node_Action {
    public:
      Friendly_Node_Action(const std::string_view name_, const std::shared_ptr<const Node_Key> node_key_, const std::shared_ptr<Node> &input, const std::shared_ptr<const Variable_Indices> &variable_indices, const std::shared_ptr<const Action> action, const std::shared_ptr<const Action> retraction) : Node_Action(name_, node_key_, input, variable_indices, action, retraction) {}
    };

    const auto action_fun = std::shared_ptr<Friendly_Node_Action>(new Friendly_Node_Action(name, node_key, input, variable_indices, action, retraction));

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

  std::shared_ptr<const Variable_Indices> Node_Action::get_variable_indices() const {
    return m_variable_indices;
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
    const auto[result, snapshot, value] = m_action_trie.insert(std::make_shared<Data>(m_variable_indices, message.token));

    if (result == Action_Trie::Result::First_Insertion) {
      (*m_action)(message.network, message.get_Job_Queue(), std::static_pointer_cast<Node_Action>(shared_from_this()), value);

      if (value->state.state.fetch_add(1, std::memory_order_release) == Data::State::STATE_INTERMEDIATE)
        (*m_retraction)(message.network, message.get_Job_Queue(), std::static_pointer_cast<Node_Action>(shared_from_this()), value);
    }
  }

  void Node_Action::receive(const Message_Token_Remove &message) {
    const auto[result, snapshot, value] = m_action_trie.erase(std::make_shared<Data>(m_variable_indices, message.token));

    if (result == Action_Trie::Result::Last_Removal) {
      if (value->state.state.fetch_add(1, std::memory_order_acquire) == Data::State::STATE_INTERMEDIATE)
        (*m_retraction)(message.network, message.get_Job_Queue(), std::static_pointer_cast<Node_Action>(shared_from_this()), value);
    }
  }

  bool Node_Action::operator==(const Node &rhs) const {
    return this == &rhs;
  }

}
