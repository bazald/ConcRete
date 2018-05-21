#ifndef ZENI_RETE_NODE_ACTION_H
#define ZENI_RETE_NODE_ACTION_H

#include "Node.hpp"

#include <functional>

namespace Zeni {

  namespace Rete {

    class Node_Action : public Node {
      Node_Action(const Node_Action &);
      Node_Action & operator=(const Node_Action &);

      friend ZENI_RETE_LINKAGE void bind_to_action(const std::shared_ptr<Network> &network, const std::shared_ptr<Node_Action> &action, const std::shared_ptr<Node> &out, const std::shared_ptr<const Variable_Indices> &variables);
      friend class Node_Action_to_Agenda;

    public:
      typedef std::function<void(const Node_Action &rete_action, const Token &token)> Action;

    private:
      Node_Action(const std::string &name_,
        const Action &action_ = [](const Node_Action &, const Token &) {},
        const Action &retraction_ = [](const Node_Action &, const Token &) {});

    public:
      ZENI_RETE_LINKAGE static std::shared_ptr<Node_Action> Create(const std::shared_ptr<Network> &network, const std::string &name, const bool &user_action, const std::shared_ptr<Node> &out, const std::shared_ptr<const Variable_Indices> &variables, const Node_Action::Action &action, const Node_Action::Action &retraction = [](const Node_Action &, const Token &) {});

      ZENI_RETE_LINKAGE ~Node_Action();

      ZENI_RETE_LINKAGE void Destroy(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output = std::shared_ptr<Node>()) override;
      ZENI_RETE_LINKAGE bool is_excised() const { return excised; }

      ZENI_RETE_LINKAGE std::shared_ptr<const Node> parent_left() const override;
      ZENI_RETE_LINKAGE std::shared_ptr<const Node> parent_right() const override;
      ZENI_RETE_LINKAGE std::shared_ptr<Node> parent_left() override;
      ZENI_RETE_LINKAGE std::shared_ptr<Node> parent_right() override;

      ZENI_RETE_LINKAGE std::shared_ptr<const Node_Filter> get_filter(const int64_t &index) const override;

      ZENI_RETE_LINKAGE const Tokens & get_output_tokens() const override;
      ZENI_RETE_LINKAGE bool has_output_tokens() const override;

      ZENI_RETE_LINKAGE void insert_token(const std::shared_ptr<Network> &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &from) override;
      ZENI_RETE_LINKAGE void remove_token(const std::shared_ptr<Network> &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &from) override;

      ZENI_RETE_LINKAGE bool matches_token(const std::shared_ptr<const Token> &token) const;

      ZENI_RETE_LINKAGE void pass_tokens(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &) override;
      ZENI_RETE_LINKAGE void unpass_tokens(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &) override;

      ZENI_RETE_LINKAGE bool operator==(const Node &/*rhs*/) const override;

      ZENI_RETE_LINKAGE void print_details(std::ostream &os) const override; ///< Formatted for dot: http://www.graphviz.org/content/dot-language

      ZENI_RETE_LINKAGE void print_rule(std::ostream &os, const std::shared_ptr<const Variable_Indices> &indices = nullptr, const std::shared_ptr<const Node> &suppress = nullptr) const override;

      ZENI_RETE_LINKAGE void output_name(std::ostream &os, const int64_t &depth) const override;

      ZENI_RETE_LINKAGE bool is_active() const override;
      ZENI_RETE_LINKAGE int64_t num_input_tokens() const;

      ZENI_RETE_LINKAGE std::vector<WME> get_filter_wmes() const override;

      ZENI_RETE_LINKAGE static std::shared_ptr<Node_Action> find_existing(const Action &/*action_*/, const Action &/*retraction_*/, const std::shared_ptr<Node> &/*out*/);

      ZENI_RETE_LINKAGE const std::string & get_name() const { return name; }
      ZENI_RETE_LINKAGE const std::shared_ptr<const Variable_Indices> & get_variables() const { return variables; }

      ZENI_RETE_LINKAGE void set_action(const Action &action_) {
        action = action_;
      }

      ZENI_RETE_LINKAGE void set_retraction(const Action &retraction_) {
        retraction = retraction_;
      }

    private:
      std::weak_ptr<Node> input;
      std::shared_ptr<const Variable_Indices> variables;
      Tokens input_tokens;
      const std::string name;
      Action action;
      Action retraction;
      bool excised = false;
    };

    class ZENI_RETE_LINKAGE Node_Action_to_Agenda {
      friend class Agenda;

      static const Node_Action::Action & action(const Node_Action &rete_action) {
        return rete_action.action;
      }

      static const Node_Action::Action & retraction(const Node_Action &rete_action) {
        return rete_action.retraction;
      }
    };

  }

}

#endif
