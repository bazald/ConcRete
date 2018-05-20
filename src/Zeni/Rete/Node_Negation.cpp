#include "Zeni/Rete/Node_Negation.hpp"

#include "Zeni/Rete/Node_Action.hpp"

#include <cassert>

namespace Zeni {

  namespace Rete {

    Node_Negation::Node_Negation()
      : output_token(std::make_shared<Token>())
    {
      output_tokens.insert(output_token);
    }

    void Node_Negation::destroy(Network &network, const std::shared_ptr<Node> &output) {
      erase_output(output);
      if (!destruction_suppressed && outputs_all.empty()) {
        //std::cerr << "Destroying: ";
        //output_name(std::cerr, 3);
        //std::cerr << std::endl;

        input.lock()->destroy(network, shared_from_this());
      }
    }

    std::shared_ptr<const Node_Filter> Node_Negation::get_filter(const int64_t &index) const {
      return parent_left()->get_filter(index);
    }

    const Node::Tokens & Node_Negation::get_output_tokens() const {
      return output_tokens;
    }

    bool Node_Negation::has_output_tokens() const {
      return input_tokens.empty();
    }

    void Node_Negation::insert_token(Network &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &
#ifndef NDEBUG
      from
#endif
    ) {
      assert(from == input.lock());

      input_tokens.insert(token);

      if (input_tokens.size() == 1) {
        const auto sft = shared_from_this();
        for (auto ot = outputs_enabled.begin(), oend = outputs_enabled.end(); ot != oend; ) {
          if ((*ot)->remove_token(network, output_token, sft))
            (*ot++)->disconnect(network, sft);
          else
            ++ot;
        }
        output_tokens.clear();
      }
    }

    bool Node_Negation::remove_token(Network &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &
#ifndef NDEBUG
      from
#endif
    ) {
      assert(from == input.lock());

      auto found = input_tokens.find(token);
      if (found != input_tokens.end()) {
        input_tokens.erase(found);
        if (input_tokens.empty()) {
          output_tokens.insert(output_token);
          const auto sft = shared_from_this();
          for (auto &output : outputs_enabled)
            output->insert_token(network, output_token, sft);
        }
      }

      return input_tokens.empty();
    }

    void Node_Negation::pass_tokens(Network &network, const std::shared_ptr<Node> &output) {
      if (input_tokens.empty())
        output->insert_token(network, output_token, shared_from_this());
    }

    void Node_Negation::unpass_tokens(Network &network, const std::shared_ptr<Node> &output) {
      if (input_tokens.empty())
        output->remove_token(network, output_token, shared_from_this());
    }

    bool Node_Negation::operator==(const Node &rhs) const {
      if (auto negation = dynamic_cast<const Node_Negation *>(&rhs))
        return input.lock() == negation->input.lock();
      return false;
    }

    void Node_Negation::print_details(std::ostream &os) const {
      os << "  " << intptr_t(this) << " [label=\"&not;&exist;\"];" << std::endl;
      os << "  " << intptr_t(input.lock().get()) << " -> " << intptr_t(this) << " [color=red];" << std::endl;
    }

    void Node_Negation::print_rule(std::ostream &os, const std::shared_ptr<const Variable_Indices> &indices, const std::shared_ptr<const Node> &suppress) const {
      if (suppress && this == suppress->parent_left().get()) {
        os << '&' << std::dynamic_pointer_cast<const Node_Action>(suppress)->get_name();
        return;
      }

      os << '-';
      const bool pb = get_token_size() > 1;
      if (pb)
        os << '}';

      parent_right()->print_rule(os, indices, suppress);

      if (pb)
        os << '}';
    }

    void Node_Negation::output_name(std::ostream &os, const int64_t &depth) const {
      os << "n(";
      const auto input_locked = input.lock();
      if (input_locked && depth)
        input_locked->output_name(os, depth - 1);
      os << ')';
    }

    bool Node_Negation::is_active() const {
      return input_tokens.empty();
    }

    std::vector<WME> Node_Negation::get_filter_wmes() const {
      return input.lock()->get_filter_wmes();
    }

    std::shared_ptr<Node_Negation> Node_Negation::find_existing(const std::shared_ptr<Node> &out) {
      for (auto &o : out->get_outputs_all()) {
        if (auto existing_negation = std::dynamic_pointer_cast<Node_Negation>(o))
          return existing_negation;
      }

      return nullptr;
    }

    void bind_to_negation(Network &network, const std::shared_ptr<Node_Negation> &negation, const std::shared_ptr<Node> &out) {
      assert(negation);
      negation->input = out;
      negation->height = out->get_height() + 1;
      negation->token_owner = out->get_token_owner();
      negation->size = out->get_size();
      negation->token_size = out->get_token_size();
      out->insert_output_enabled(negation);
      out->pass_tokens(network, negation);
    }

  }

}
