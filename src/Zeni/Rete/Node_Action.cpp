#include "Zeni/Rete/Node_Action.hpp"

#include "Zeni/Rete/Network.hpp"

namespace Zeni {

  namespace Rete {

    Node_Action::Node_Action(const std::string &name_,
      const Action &action_,
      const Action &retraction_)
      : name(name_),
      action(action_),
      retraction(retraction_)
    {
      assert(!name.empty());
    }

    Node_Action::~Node_Action() {
      //    if(!excised) {
      for (auto &token : input_tokens)
        retraction(*this, *token);
      //    }
    }

    void Node_Action::destroy(Network &network, const std::shared_ptr<Node> &
#ifndef NDEBUG
      output
#endif
    ) {
      assert(!output);

      if (!destruction_suppressed && !excised) {
        excised = true;
        network.excise_rule(name, false);

        //std::cerr << "Destroying: ";
        //output_name(std::cerr);
        //std::cerr << std::endl;

        for (auto &token : input_tokens)
          retraction(*this, *token);

        input.lock()->destroy(network, shared_from_this());
      }
    }

    std::shared_ptr<const Node> Node_Action::parent_left() const { return input.lock(); }
    std::shared_ptr<const Node> Node_Action::parent_right() const { return input.lock(); }
    std::shared_ptr<Node> Node_Action::parent_left() { return input.lock(); }
    std::shared_ptr<Node> Node_Action::parent_right() { return input.lock(); }

    std::shared_ptr<const Node_Filter> Node_Action::get_filter(const int64_t &index) const {
      return parent_left()->get_filter(index);
    }

    const Node::Tokens & Node_Action::get_output_tokens() const {
      abort();
    }

    bool Node_Action::has_output_tokens() const {
      abort();
    }

    void Node_Action::insert_token(Network &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &
#ifndef NDEBUG
      from
#endif
    ) {
      assert(from == input.lock());

      const auto inserted = input_tokens.insert(token);

      //#ifdef DEBUG_OUTPUT
      //    std::cerr << "Firing action: " << get_name() << " on ";
      //    token->print(std::cerr);
      //    std::cerr << std::endl;
      //#endif

      network.get_agenda().insert_action(std::static_pointer_cast<Node_Action>(shared_from_this()), *inserted.first);
    }

    bool Node_Action::remove_token(Network &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &
#ifndef NDEBUG
      from
#endif
    ) {
      assert(from == input.lock());

      //#ifdef DEBUG_OUTPUT
      //    std::cerr << "Retracting action: " << get_name() << " on ";
      //    token->print(std::cerr);
      //    std::cerr << std::endl;
      //#endif

      auto found = input_tokens.find(token);
      if (found != input_tokens.end())
        // TODO: change from the 'if' to the 'assert', ensuring that we're not wasting time on non-existent removals
        //assert(found != input_tokens.end());
      {
        network.get_agenda().insert_retraction(std::static_pointer_cast<Node_Action>(shared_from_this()), *found);

        input_tokens.erase(found);
      }

      return input_tokens.empty();
    }

    bool Node_Action::matches_token(const std::shared_ptr<const Token> &token) const {
      return input_tokens.find(token) != input_tokens.end();
    }

    void Node_Action::pass_tokens(Network &, const std::shared_ptr<Node> &) {
      abort();
    }

    void Node_Action::unpass_tokens(Network &, const std::shared_ptr<Node> &) {
      abort();
    }

    bool Node_Action::operator==(const Node &/*rhs*/) const {
      //       if(autoZENI_RETE_action = dynamic_cast<const Node_Action *>(&rhs))
      //         return action ==ZENI_RETE_action->action && retraction ==ZENI_RETE_action->retraction && input ==ZENI_RETE_action->input;
      return false;
    }

    void Node_Action::print_details(std::ostream &os) const {
      os << "  " << intptr_t(this) << " [label=\"Act\"];" << std::endl;
      os << "  " << intptr_t(input.lock().get()) << " -> " << intptr_t(this) << " [color=red];" << std::endl;
    }

    void Node_Action::print_rule(std::ostream &os, const std::shared_ptr<const Variable_Indices> &, const std::shared_ptr<const Node> &) const {
#ifdef DEBUG_OUTPUT
      {
        const auto tokens = parent_left()->get_output_tokens();

        os << "# Matches: " << tokens.size() << std::endl;

        size_t matches = 0;
        for (const auto &token : tokens)
          os << "# Match " << ++matches << ": " << *token << std::endl;

        os << "# Variables: " << *variables << std::endl;
      }
#endif

      const std::shared_ptr<const Node> suppress = custom_data->get_suppress();

      os << "sp {" << name;
      if (custom_data)
        custom_data->print_flags(os);
      os << std::endl << "  ";

      parent_left()->print_rule(os, variables, suppress);

      os << std::endl << "-->";
      if (custom_data) {
        os << std::endl;
        custom_data->print_action(os);
      }
      os << std::endl << '}' << std::endl;
    }

    void Node_Action::output_name(std::ostream &os, const int64_t &depth) const {
      os << "a(";
      const auto input_locked = input.lock();
      if (input_locked && depth)
        input_locked->output_name(os, depth - 1);
      os << ')';
    }

    bool Node_Action::is_active() const {
      //    return !parent_left()->get_output_tokens().empty();
      return !input_tokens.empty();
    }

    int64_t Node_Action::num_input_tokens() const {
      return int64_t(input_tokens.size());
    }

    std::vector<WME> Node_Action::get_filter_wmes() const {
      return input.lock()->get_filter_wmes();
    }

    std::shared_ptr<Node_Action> Node_Action::find_existing(const Action &/*action_*/, const Action &/*retraction_*/, const std::shared_ptr<Node> &/*out*/) {
      //       for(auto &o : out->get_outputs()) {
      //         if(auto existing_action = std::dynamic_pointer_cast<Node_Action>(o)) {
      //           if(action_ == existing_action->action && retraction_ == existing_action->retraction)
      //             return existing_action;
      //         }
      //       }

      return nullptr;
    }

    void bind_to_action(Network &network, const std::shared_ptr<Node_Action> &action, const std::shared_ptr<Node> &out, const std::shared_ptr<const Variable_Indices> &variables) {
      assert(action && !action->input.lock());
      action->input = out;
      action->variables = variables;
      action->height = out->get_height() + 1;
      action->token_owner = out->get_token_owner();
      action->size = out->get_size();
      action->token_size = out->get_token_size();
      out->insert_output_enabled(action);
      out->pass_tokens(network, action);
    }

  }

}
