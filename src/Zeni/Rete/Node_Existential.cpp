#include "Zeni/Rete/Node_Existential.hpp"

#include "Zeni/Rete/Node_Action.hpp"

#include <cassert>

namespace Zeni {

  namespace Rete {

    Node_Existential::Node_Existential() : output_token(std::make_shared<Token>()) {}

    void Node_Existential::destroy(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output) {
      erase_output(output);
      if (!destruction_suppressed && outputs_all.empty()) {
        //std::cerr << "Destroying: ";
        //output_name(std::cerr, 3);
        //std::cerr << std::endl;

        input.lock()->destroy(network, shared_from_this());
      }
    }

    std::shared_ptr<const Node> Node_Existential::parent_left() const { return input.lock(); }
    std::shared_ptr<const Node> Node_Existential::parent_right() const { return input.lock(); }
    std::shared_ptr<Node> Node_Existential::parent_left() { return input.lock(); }
    std::shared_ptr<Node> Node_Existential::parent_right() { return input.lock(); }

    std::shared_ptr<const Node_Filter> Node_Existential::get_filter(const int64_t &index) const {
      return parent_left()->get_filter(index);
    }

    const Node::Tokens & Node_Existential::get_output_tokens() const {
      return output_tokens;
    }

    bool Node_Existential::has_output_tokens() const {
      return !input_tokens.empty();
    }

    void Node_Existential::insert_token(const std::shared_ptr<Network> &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &
#ifndef NDEBUG
      from
#endif
    ) {
      assert(from == input.lock());

      input_tokens.insert(token);

      if (input_tokens.size() == 1) {
        const auto sft = shared_from_this();
        for (auto &output : outputs_enabled)
          output->insert_token(network, output_token, sft);
        output_tokens.insert(output_token);
      }

      //std::cerr << "input_tokens.size() == " << input_tokens.size() << std::endl;
    }

    bool Node_Existential::remove_token(const std::shared_ptr<Network> &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &
#ifndef NDEBUG
      from
#endif
    ) {
      assert(from == input.lock());

      auto found = input_tokens.find(token);
      if (found != input_tokens.end()) {
        input_tokens.erase(found);
        if (input_tokens.empty()) {
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

      //std::cerr << "input_tokens.size() == " << input_tokens.size() << std::endl;

      return input_tokens.empty();
    }

    void Node_Existential::pass_tokens(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output) {
      if (!input_tokens.empty())
        output->insert_token(network, output_token, shared_from_this());
    }

    void Node_Existential::unpass_tokens(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output) {
      if (!input_tokens.empty())
        output->remove_token(network, output_token, shared_from_this());
    }

    bool Node_Existential::operator==(const Node &rhs) const {
      if (auto existential = dynamic_cast<const Node_Existential *>(&rhs))
        return input.lock() == existential->input.lock();
      return false;
    }

    void Node_Existential::print_details(std::ostream &os) const {
      os << "  " << intptr_t(this) << " [label=\"&exist;\"];" << std::endl;
      os << "  " << intptr_t(input.lock().get()) << " -> " << intptr_t(this) << " [color=red];" << std::endl;
    }

    void Node_Existential::print_rule(std::ostream &os, const std::shared_ptr<const Variable_Indices> &indices, const std::shared_ptr<const Node> &suppress) const {
      if (suppress && this == suppress->parent_left().get()) {
        os << '&' << std::dynamic_pointer_cast<const Node_Action>(suppress)->get_name();
        return;
      }

      os << '+';
      const bool pb = get_token_size() > 1;
      if (pb)
        os << '{';

      parent_right()->print_rule(os, indices, suppress);

      if (pb)
        os << '}';
    }

    void Node_Existential::output_name(std::ostream &os, const int64_t &depth) const {
      os << "e(";
      const auto input_locked = input.lock();
      if (input_locked && depth)
        input_locked->output_name(os, depth - 1);
      os << ')';
    }

    bool Node_Existential::is_active() const {
      return !input_tokens.empty();
    }

    std::vector<WME> Node_Existential::get_filter_wmes() const {
      return input.lock()->get_filter_wmes();
    }

    std::shared_ptr<Node_Existential> Node_Existential::find_existing(const std::shared_ptr<Node> &out) {
      for (auto &o : out->get_outputs_all()) {
        if (auto existing_existential = std::dynamic_pointer_cast<Node_Existential>(o))
          return existing_existential;
      }

      return nullptr;
    }

    void bind_to_existential(const std::shared_ptr<Network> &network, const std::shared_ptr<Node_Existential> &existential, const std::shared_ptr<Node> &out) {
      assert(existential);
      existential->input = out;
      existential->height = out->get_height() + 1;
      existential->token_owner = out->get_token_owner();
      existential->size = out->get_size();
      existential->token_size = out->get_token_size();
      out->insert_output_enabled(existential);
      out->pass_tokens(network, existential);
    }

  }

}
