#ifndef ZENI_RETE_NODE_ACTION_HPP
#define ZENI_RETE_NODE_ACTION_HPP

#include "Internal/Node_Unary.hpp"

#include <functional>

namespace Zeni::Rete {

  class Node_Action : public Node_Unary {
    Node_Action(const Node_Action &) = delete;
    Node_Action & operator=(const Node_Action &) = delete;

  public:
    typedef std::function<void(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token)> Action;

    struct Compare_By_Name_Eq {
      bool operator()(const std::shared_ptr<Node_Action> &lhs, const std::shared_ptr<Node_Action> &rhs) const noexcept {
        return lhs->get_name() == rhs->get_name();
      }

      bool operator()(const Node_Action &lhs, const Node_Action &rhs) const noexcept {
        return lhs.get_name() == rhs.get_name();
      }

      bool operator()(const std::string_view &lhs, const std::string_view &rhs) const noexcept {
        return lhs == rhs;
      }

      bool operator()(const std::shared_ptr<Node_Action> &lhs, const std::string_view &rhs) const noexcept {
        return lhs->get_name() == rhs;
      }

      bool operator()(const std::string_view &lhs, const std::shared_ptr<Node_Action> &rhs) const noexcept {
        return lhs == rhs->get_name();
      }

      bool operator()(const Node_Action &lhs, const std::string_view &rhs) const noexcept {
        return lhs.get_name() == rhs;
      }

      bool operator()(const std::string_view &lhs, const Node_Action &rhs) const noexcept {
        return lhs == rhs.get_name();
      }
    };

    struct Hash_By_Name {
      size_t operator()(const std::shared_ptr<Node_Action> &rete_action) const noexcept {
        return std::hash<std::string_view>()(rete_action->get_name());
      }

      size_t operator()(const Node_Action &rete_action) const noexcept {
        return std::hash<std::string_view>()(rete_action.get_name());
      }

      size_t operator()(const std::string_view &name) const noexcept {
        return std::hash<std::string_view>()(name);
      }
    };

  private:
    Node_Action(const std::string_view name, const std::shared_ptr<const Node_Key> node_key, const std::shared_ptr<Node> input, const std::shared_ptr<const Variable_Indices> variable_indices, const Action &action_, const Action &retraction_);

  public:
    Node_Action(const std::string_view name);

    ZENI_RETE_LINKAGE static std::shared_ptr<Node_Action> Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::string_view name, const bool user_action, const std::shared_ptr<const Node_Key> node_key, const std::shared_ptr<Node> input, const std::shared_ptr<const Variable_Indices> variable_indices, const Node_Action::Action action, const Node_Action::Action retraction = [](const std::shared_ptr<Network>, const std::shared_ptr<Concurrency::Job_Queue>, const std::shared_ptr<Node_Action>, const std::shared_ptr<const Token>) {});

    ZENI_RETE_LINKAGE ~Node_Action();

    ZENI_RETE_LINKAGE void Destroy(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue);

    ZENI_RETE_LINKAGE std::string get_name() const;
    ZENI_RETE_LINKAGE std::shared_ptr<const Variable_Indices> get_variable_indices() const;

    ZENI_RETE_LINKAGE std::pair<Node_Trie::Result, std::shared_ptr<Node>> connect_new_or_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) override;
    ZENI_RETE_LINKAGE Node_Trie::Result connect_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) override;

    ZENI_RETE_LINKAGE void receive(const Message_Token_Insert &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Token_Remove &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Disconnect_Output &message) override;

    ZENI_RETE_LINKAGE bool operator==(const Node &) const override;

  private:
    std::shared_ptr<const Variable_Indices> m_variable_indices;
    const std::string m_name;
    Action m_action;
    Action m_retraction;

    Token_Trie m_token_trie;
  };

}

#endif
