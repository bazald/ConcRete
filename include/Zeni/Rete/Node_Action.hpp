#ifndef ZENI_RETE_NODE_ACTION_HPP
#define ZENI_RETE_NODE_ACTION_HPP

#include "Node_Unary.hpp"

#include <functional>

namespace Zeni::Rete {

  class Node_Action : public Node_Unary {
    Node_Action(const Node_Action &) = delete;
    Node_Action & operator=(const Node_Action &) = delete;

  public:
    typedef std::function<void(const Node_Action &rete_action, const Token &token)> Action;

  private:
    Node_Action(const std::string_view name_, const std::shared_ptr<Node> input, const std::shared_ptr<const Variable_Indices> variables,
      const Action &action_ = [](const Node_Action &, const Token &) {},
      const Action &retraction_ = [](const Node_Action &, const Token &) {});

  public:
    ZENI_RETE_LINKAGE static std::shared_ptr<Node_Action> Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::string_view name, const bool user_action, const std::shared_ptr<Node> input, const std::shared_ptr<const Variable_Indices> variables, const Node_Action::Action action, const Node_Action::Action retraction = [](const Node_Action &, const Token &) {});

    ZENI_RETE_LINKAGE ~Node_Action();

    ZENI_RETE_LINKAGE std::string get_name() const;
    ZENI_RETE_LINKAGE std::shared_ptr<const Variable_Indices> get_variables() const;

    ZENI_RETE_LINKAGE void receive(const Raven_Status_Empty &) override;
    ZENI_RETE_LINKAGE void receive(const Raven_Status_Nonempty &) override;
    ZENI_RETE_LINKAGE void receive(const Raven_Token_Insert &raven) override;
    ZENI_RETE_LINKAGE void receive(const Raven_Token_Remove &raven) override;

    ZENI_RETE_LINKAGE bool operator==(const Node &) const override;

  private:
    std::shared_ptr<const Variable_Indices> m_variables;
    const std::string m_name;
    Action m_action;
    Action m_retraction;
  };

}

#endif
