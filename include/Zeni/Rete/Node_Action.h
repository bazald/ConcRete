#ifndef RETE_ACTION_H
#define RETE_ACTION_H

#include "Node.h"

#include <functional>

namespace Zeni {

  namespace Rete {

    class ZENI_RETE_LINKAGE Node_Action : public Node {
      Node_Action(const Node_Action &);
      Node_Action & operator=(const Node_Action &);

      friend ZENI_RETE_LINKAGE void bind_to_action(Network &network, const std::shared_ptr<Node_Action> &action, const std::shared_ptr<Node> &out, const std::shared_ptr<const Variable_Indices> &variables);
      friend class Node_Action_to_Agenda;

    public:
      typedef std::function<void(const Node_Action &rete_action, const Token &token)> Action;

      Node_Action(const std::string &name_,
        const Action &action_ = [](const Node_Action &, const Token &) {},
        const Action &retraction_ = [](const Node_Action &, const Token &) {});

      ~Node_Action();

      void destroy(Network &network, const std::shared_ptr<Node> &output = std::shared_ptr<Node>()) override;
      bool is_excised() const { return excised; }

      std::shared_ptr<const Node> parent_left() const override;
      std::shared_ptr<const Node> parent_right() const override;
      std::shared_ptr<Node> parent_left() override;
      std::shared_ptr<Node> parent_right() override;

      std::shared_ptr<const Node_Filter> get_filter(const int64_t &index) const override;

      const Tokens & get_output_tokens() const override;
      bool has_output_tokens() const override;

      void insert_token(Network &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &from) override;
      bool remove_token(Network &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &from) override;
      bool matches_token(const std::shared_ptr<const Token> &token) const;

      void pass_tokens(Network &network, const std::shared_ptr<Node> &) override;
      void unpass_tokens(Network &network, const std::shared_ptr<Node> &) override;

      bool operator==(const Node &/*rhs*/) const override;

      void print_details(std::ostream &os) const override; ///< Formatted for dot: http://www.graphviz.org/content/dot-language

      void print_rule(std::ostream &os, const std::shared_ptr<const Variable_Indices> &indices = nullptr, const std::shared_ptr<const Node> &suppress = nullptr) const override;

      void output_name(std::ostream &os, const int64_t &depth) const override;

      bool is_active() const override;
      int64_t num_input_tokens() const;

      std::vector<WME> get_filter_wmes() const override;

      static std::shared_ptr<Node_Action> find_existing(const Action &/*action_*/, const Action &/*retraction_*/, const std::shared_ptr<Node> &/*out*/);

      const std::string & get_name() const { return name; }
      const std::shared_ptr<const Variable_Indices> & get_variables() const { return variables; }

      void set_action(const Action &action_) {
        action = action_;
      }

      void set_retraction(const Action &retraction_) {
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

    ZENI_RETE_LINKAGE void bind_to_action(Network &network, const std::shared_ptr<Node_Action> &action, const std::shared_ptr<Node> &out, const std::shared_ptr<const Variable_Indices> &variables);

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
