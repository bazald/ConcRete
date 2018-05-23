#ifndef ZENI_RETE_NODE_ACTION_H
#define ZENI_RETE_NODE_ACTION_H

#include "Node_Unary.hpp"

#include <functional>

namespace Zeni {

  namespace Rete {

    class Node_Action : public Node_Unary {
      Node_Action(const Node_Action &) = delete;
      Node_Action & operator=(const Node_Action &) = delete;

    public:
      typedef std::function<void(const Node_Action &rete_action, const Token &token)> Action;

    private:
      Node_Action(const std::string &name_, const std::shared_ptr<Node> &input, const std::shared_ptr<const Variable_Indices> &variables,
        const Action &action_ = [](const Node_Action &, const Token &) {},
        const Action &retraction_ = [](const Node_Action &, const Token &) {});

    public:
      ZENI_RETE_LINKAGE static std::shared_ptr<Node_Action> Create_Or_Increment_Output_Count(const std::shared_ptr<Network> &network, const std::string &name, const bool &user_action, const std::shared_ptr<Node> &input, const std::shared_ptr<const Variable_Indices> &variables, const Node_Action::Action &action, const Node_Action::Action &retraction = [](const Node_Action &, const Token &) {});

      ZENI_RETE_LINKAGE ~Node_Action();

      ZENI_RETE_LINKAGE const std::string & get_name() const { return m_name; }
      ZENI_RETE_LINKAGE const std::shared_ptr<const Variable_Indices> & get_variables() const { return m_variables; }

      ZENI_RETE_LINKAGE bool receive(const Raven_Token_Insert &raven) override;
      ZENI_RETE_LINKAGE bool receive(const Raven_Token_Remove &raven) override;

      ZENI_RETE_LINKAGE bool operator==(const Node &) const override;

    private:
      std::shared_ptr<const Variable_Indices> m_variables;
      const std::string m_name;
      Action m_action;
      Action m_retraction;
    };

  }

}

#endif
