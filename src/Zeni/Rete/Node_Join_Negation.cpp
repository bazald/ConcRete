#include "Zeni/Rete/Node_Join_Negation.h"

#include "Zeni/Rete/Node_Action.h"

#include <cassert>

namespace Zeni {

  namespace Rete {

    Node_Join_Negation::Node_Join_Negation(Variable_Bindings bindings_) : bindings(bindings_) {}

    void Node_Join_Negation::destroy(Network &network, const std::shared_ptr<Node> &output) {
      erase_output(output);
      if (!destruction_suppressed && outputs_all.empty()) {
        //std::cerr << "Destroying: ";
        //output_name(std::cerr, 3);
        //std::cerr << std::endl;

        auto input0_locked = input0.lock();
        auto input1_locked = input1.lock();
        auto sft = shared_from_this();
        input0_locked->destroy(network, sft);
        if (input0_locked != input1_locked)
          input1_locked->destroy(network, sft);
      }
    }

    std::shared_ptr<const Node_Filter> Node_Join_Negation::get_filter(const int64_t &index) const {
      const int64_t left_size = parent_left()->get_token_size();
      if (index < left_size)
        return parent_left()->get_filter(index);
      else
        return parent_right()->get_filter(index - left_size);
    }

    const Node::Tokens & Node_Join_Negation::get_output_tokens() const {
      return output_tokens;
    }

    bool Node_Join_Negation::has_output_tokens() const {
      return !output_tokens.empty();
    }

    void Node_Join_Negation::insert_token(Network &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &from) {
      const auto input0_locked = input0.lock();
      const auto input1_locked = input1.lock();

      assert(from == input0_locked || from == input1_locked);

      if (from == input0_locked) {
        auto &match = matching[std::make_pair(token, true)];
        const auto inserted = match.first.insert(token);
        if (inserted.second) {
          ++input0_count;
          if (match.second.empty())
            join_tokens(network, *inserted.first);
        }
      }
      if (from == input1_locked) {
        auto &match = matching[std::make_pair(token, false)];
        const auto inserted = match.second.insert(token);
        if (inserted.second) {
          ++input1_count;
          if (match.second.size() == 1u) {
            for (const auto &other : match.first)
              unjoin_tokens(network, other);
          }
        }
      }
    }

    bool Node_Join_Negation::remove_token(Network &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &from) {
      const auto input0_locked = input0.lock();
      const auto input1_locked = input1.lock();

      assert(from == input0_locked || from == input1_locked);

      bool emptied = false;

      if (from == input0_locked) {
        const auto index = std::make_pair(token, true);
        auto &match = matching[index];
        auto found2 = match.first.find(token);

        if (found2 != match.first.end()) {
          if (match.second.empty())
            unjoin_tokens(network, *found2);
          match.first.erase(found2);

          emptied ^= !--input0_count;
        }

        if (match.first.empty() && match.second.empty())
          matching.erase(index);
      }
      if (from == input1_locked) {
        const auto index = std::make_pair(token, false);
        auto &match = matching[index];
        auto found2 = match.second.find(token);

        if (found2 != match.second.end()) {
          if (match.second.size() == 1) {
            for (auto &other : match.first)
              join_tokens(network, other);
          }
          match.second.erase(found2);

          emptied ^= !--input1_count;
        }

        if (match.second.empty() && match.first.empty())
          matching.erase(index);
      }

      return emptied;
    }

    bool Node_Join_Negation::operator==(const Node &rhs) const {
      if (auto join = dynamic_cast<const Node_Join_Negation *>(&rhs))
        return bindings == join->bindings && input0.lock() == join->input0.lock() && input1.lock() == join->input1.lock();
      return false;
    }

    void Node_Join_Negation::print_details(std::ostream &os) const {
      os << "  " << intptr_t(this) << " [label=\"&not;&exist;J\\n";
      if (bindings.empty())
        os << "&empty;";
      else {
        auto bt = bindings.begin();
        os << *bt++;
        while (bt != bindings.end())
          os << "\\n" << *bt++;
      }
      os << "\"];" << std::endl;

      os << "  " << intptr_t(input0.lock().get()) << " -> " << intptr_t(this) << " [color=red];" << std::endl;
      os << "  " << intptr_t(input1.lock().get()) << " -> " << intptr_t(this) << " [color=blue];" << std::endl;
    }

    void Node_Join_Negation::print_rule(std::ostream &os, const std::shared_ptr<const Variable_Indices> &indices, const std::shared_ptr<const Node> &suppress) const {
      if (suppress && this == suppress->parent_left().get()) {
        os << '&' << std::dynamic_pointer_cast<const Node_Action>(suppress)->get_name();
        return;
      }

      const auto pl = parent_left();
      const auto pr = parent_right();
      const bool prb = !dynamic_cast<const Node_Filter *>(pr.get());

      pl->print_rule(os, indices, suppress);
      os << std::endl << "  ";

      os << '-';
      if (prb)
        os << '{';

      const auto bound = bind_Variable_Indices(bindings, indices, *pl, *pr);

      pr->print_rule(os, bound, suppress);

      if (prb)
        os << '}';
    }

    void Node_Join_Negation::output_name(std::ostream &os, const int64_t &depth) const {
      os << "nj(" << bindings << ',';
      const auto input0_locked = input0.lock();
      if (input0_locked && depth)
        input0_locked->output_name(os, depth - 1);
      os << ',';
      const auto input1_locked = input1.lock();
      if (input1_locked && depth)
        input1_locked->output_name(os, depth - 1);
      os << ')';
    }

    bool Node_Join_Negation::is_active() const {
      return Node_Join_Negation::has_output_tokens();
    }

    std::vector<WME> Node_Join_Negation::get_filter_wmes() const {
      auto filter_wmes0 = input0.lock()->get_filter_wmes();
      auto filter_wmes1 = input1.lock()->get_filter_wmes();
      filter_wmes0.insert(filter_wmes0.end(), filter_wmes1.begin(), filter_wmes1.end());
      return filter_wmes0;
    }

    std::shared_ptr<Node_Join_Negation> Node_Join_Negation::find_existing(const Variable_Bindings &bindings, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1) {
      for (auto &o0 : out0->get_outputs_all()) {
        if (auto existing_negation_join = std::dynamic_pointer_cast<Node_Join_Negation>(o0)) {
          if (std::find(out1->get_outputs_all().begin(), out1->get_outputs_all().end(), existing_negation_join) != out1->get_outputs_all().end()) {
            if (bindings == existing_negation_join->bindings)
              return existing_negation_join;
          }
        }
      }

      return nullptr;
    }

    void Node_Join_Negation::join_tokens(Network &network, const std::shared_ptr<const Token> &lhs) {
      //    for(auto &binding : bindings) {
      //      if(*(*lhs.first)[binding.first] != *(*rhs)[binding.second])
      //        return;
      //    }

      //    if(--lhs.second == 0) {
      const auto token = output_tokens.insert(lhs);

      const auto sft = shared_from_this();
      for (auto &output : outputs_enabled)
        output->insert_token(network, *token.first, sft);
      //    }
    }

    void Node_Join_Negation::unjoin_tokens(Network &network, const std::shared_ptr<const Token> &lhs) {
      //    for(auto &binding : bindings) {
      //      if(*(*lhs.first)[binding.first] != *(*rhs)[binding.second])
      //        return;
      //    }

      //    if(++lhs.second == 1) {
      const auto sft = shared_from_this();
      for (auto ot = outputs_enabled.begin(), oend = outputs_enabled.end(); ot != oend; ) {
        if ((*ot)->remove_token(network, lhs, sft))
          (*ot++)->disconnect(network, sft);
        else
          ++ot;
      }

      output_tokens.erase(lhs);
      //    }
    }

    void Node_Join_Negation::pass_tokens(Network &network, const std::shared_ptr<Node> &output) {
      const auto sft = shared_from_this();
      for (auto &token : output_tokens)
        output->insert_token(network, token, sft);
    }

    void Node_Join_Negation::unpass_tokens(Network &network, const std::shared_ptr<Node> &output) {
      const auto sft = shared_from_this();
      for (auto &token : output_tokens)
        output->remove_token(network, token, sft);
    }

    void bind_to_negation_join(Network &network, const std::shared_ptr<Node_Join_Negation> &join, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1) {
      assert(join && !join->input0.lock() && !join->input1.lock());
      join->input0 = out0;
      join->input1 = out1;
      join->height = std::max(out0->get_height(), out1->get_height()) + 1;
      join->token_owner = out0->get_token_owner();
      join->size = out0->get_size() + out1->get_size();
      join->token_size = out0->get_token_size();

      out0->insert_output_enabled(join);
      out0->pass_tokens(network, join);
      if (out0 != out1) {
        out1->insert_output_enabled(join);
        out1->pass_tokens(network, join);
      }
    }

  }

}
