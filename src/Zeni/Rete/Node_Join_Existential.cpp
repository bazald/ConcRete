#include "Zeni/Rete/Node_Join_Existential.hpp"

#include "Zeni/Rete/Node_Action.hpp"

#include <cassert>

#undef ZENI_RETE_LR_UNLINKING

namespace Zeni {

  namespace Rete {

    Node_Join_Existential::Node_Join_Existential(Variable_Bindings bindings_)
      : bindings(bindings_)
    {
    }

    void Node_Join_Existential::destroy(Network &network, const std::shared_ptr<Node> &output) {
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

    std::shared_ptr<const Node_Filter> Node_Join_Existential::get_filter(const int64_t &index) const {
      const int64_t left_size = parent_left()->get_token_size();
      if (index < left_size)
        return parent_left()->get_filter(index);
      else
        return parent_right()->get_filter(index - left_size);
    }

    const Node::Tokens & Node_Join_Existential::get_output_tokens() const {
      return output_tokens;
    }

    bool Node_Join_Existential::has_output_tokens() const {
      return !output_tokens.empty();
    }

    void Node_Join_Existential::insert_token(Network &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &from) {
      const auto input0_locked = input0.lock();
      const auto input1_locked = input1.lock();

      assert(from == input0_locked || from == input1_locked);

      if (from == input0_locked) {
#ifdef ZENI_RETE_LR_UNLINKING
        if (!data.connected1) {
          //#ifdef DEBUG_OUTPUT
          //        std::cerr << this << " Connecting right" << std::endl;
          //#endif
          assert(!input1_count);
          input1_locked->enable_output(network, this);
          data.connected1 = true;
        }
#endif

        auto &match = matching[std::make_pair(token, true)];
        const auto inserted = match.first.insert(token);
        if (inserted.second) {
          ++input0_count;
          if (!match.second.empty())
            join_tokens(network, *inserted.first);
        }
      }
      if (from == input1_locked) {
#ifdef ZENI_RETE_LR_UNLINKING
        if (!data.connected0) {
          //#ifdef DEBUG_OUTPUT
          //        std::cerr << this << " Connecting left" << std::endl;
          //#endif
          assert(!input0_count);
          input0_locked->enable_output(network, this);
          data.connected0 = true;
        }
#endif

        auto &match = matching[std::make_pair(token, false)];
        const auto inserted = match.second.insert(token);
        if (inserted.second) {
          ++input1_count;
          if (match.second.size() == 1u) {
            for (const auto &other : match.first)
              join_tokens(network, other);
          }
        }
      }
    }

    bool Node_Join_Existential::remove_token(Network &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &from) {
      const auto input0_locked = input0.lock();
      const auto input1_locked = input1.lock();

      assert(from == input0_locked || from == input1_locked);

      bool emptied = false;

      if (from == input0_locked) {
        const auto index = std::make_pair(token, true);
        auto &match = matching[index];
        auto found2 = match.first.find(token);

        if (found2 != match.first.end()) {
          if (!match.second.empty())
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
              unjoin_tokens(network, other);
          }
          match.second.erase(found2);

          emptied ^= !--input1_count;
        }

        if (match.second.empty() && match.first.empty())
          matching.erase(index);
      }

      return emptied;
    }

    bool Node_Join_Existential::operator==(const Node &rhs) const {
      if (auto join = dynamic_cast<const Node_Join_Existential *>(&rhs))
        return bindings == join->bindings && input0.lock() == join->input0.lock() && input1.lock() == join->input1.lock();
      return false;
    }

    bool Node_Join_Existential::disabled_input(const std::shared_ptr<Node> &
#ifdef ZENI_RETE_LR_UNLINKING
      input
#endif
    ) {
#ifdef ZENI_RETE_LR_UNLINKING
      if (input.get() == input0)
        return !data.connected0;
      else {
        assert(input.get() == input1);
        return !data.connected1;
      }
#else
      return false;
#endif
    }

    void Node_Join_Existential::print_details(std::ostream &os) const {
      os << "  " << intptr_t(this) << " [label=\"&exist;J\\n";
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

    void Node_Join_Existential::print_rule(std::ostream &os, const std::shared_ptr<const Variable_Indices> &indices, const std::shared_ptr<const Node> &suppress) const {
      if (suppress && this == suppress->parent_left().get()) {
        os << '&' << std::dynamic_pointer_cast<const Node_Action>(suppress)->get_name();
        return;
      }

      const auto pl = parent_left();
      const auto pr = parent_right();
      const bool prb = !dynamic_cast<const Node_Filter *>(pr.get());

      pl->print_rule(os, indices, suppress);
      os << std::endl << "  ";

      os << '+';
      if (prb)
        os << '{';

      const auto bound = bind_Variable_Indices(bindings, indices, *pl, *pr);

      pr->print_rule(os, bound, suppress);

      if (prb)
        os << '}';
    }

    void Node_Join_Existential::output_name(std::ostream &os, const int64_t &depth) const {
      os << "ej(" << bindings << ',';
      const auto input0_locked = input0.lock();
      if (input0_locked && depth)
        input0_locked->output_name(os, depth - 1);
      os << ',';
      const auto input1_locked = input1.lock();
      if (input1_locked && depth)
        input1_locked->output_name(os, depth - 1);
      os << ')';
    }

    bool Node_Join_Existential::is_active() const {
      return Node_Join_Existential::has_output_tokens();
    }

    std::vector<WME> Node_Join_Existential::get_filter_wmes() const {
      auto filter_wmes0 = input0.lock()->get_filter_wmes();
      auto filter_wmes1 = input1.lock()->get_filter_wmes();
      filter_wmes0.insert(filter_wmes0.end(), filter_wmes1.begin(), filter_wmes1.end());
      return filter_wmes0;
    }

    std::shared_ptr<Node_Join_Existential> Node_Join_Existential::find_existing(const Variable_Bindings &bindings, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1) {
      for (auto &o0 : out0->get_outputs_all()) {
        if (auto existing_existential_join = std::dynamic_pointer_cast<Node_Join_Existential>(o0)) {
          if (std::find(out1->get_outputs_all().begin(), out1->get_outputs_all().end(), existing_existential_join) != out1->get_outputs_all().end()) {
            if (bindings == existing_existential_join->bindings)
              return existing_existential_join;
          }
        }
      }

      return nullptr;
    }

    void Node_Join_Existential::join_tokens(Network &network, const std::shared_ptr<const Token> &lhs) {
      //    for(auto &binding : bindings) {
      //      if(*(*lhs.first)[binding.first] != *(*rhs)[binding.second])
      //        return;
      //    }

      //    if(++lhs.second == 1u) {
      const auto token = output_tokens.insert(lhs);

      const auto sft = shared_from_this();
      for (auto &output : outputs_enabled)
        output->insert_token(network, *token.first, sft);
      //    }
    }

    void Node_Join_Existential::unjoin_tokens(Network &network, const std::shared_ptr<const Token> &lhs) {
      //    for(auto &binding : bindings) {
      //      if(*(*lhs.first)[binding.first] != *(*rhs)[binding.second])
      //        return;
      //    }

      //    if(--lhs.second == 0) {
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

    void Node_Join_Existential::pass_tokens(Network &network, const std::shared_ptr<Node> &output) {
      const auto sft = shared_from_this();
      for (auto &token : output_tokens)
        output->insert_token(network, token, sft);
    }

    void Node_Join_Existential::unpass_tokens(Network &network, const std::shared_ptr<Node> &output) {
      const auto sft = shared_from_this();
      for (auto &token : output_tokens)
        output->remove_token(network, token, sft);
    }

    void Node_Join_Existential::disconnect(Network &
#ifdef ZENI_RETE_LR_UNLINKING
      network
#endif
      , const std::shared_ptr<const Node> &
#ifdef ZENI_RETE_LR_UNLINKING
      from
#endif
    ) {
#ifdef ZENI_RETE_LR_UNLINKING
      if (input0 != input1) {
        if (from == input0) {
          assert(data.connected1);
          input1->disable_output(network, this);
          data.connected1 = false;
        }
        else {
          assert(data.connected0);
          input0->disable_output(network, this);
          data.connected0 = false;
        }
      }
      assert(data.connected0 || data.connected1);
#endif
    }

    void bind_to_existential_join(Network &network, const std::shared_ptr<Node_Join_Existential> &join, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1) {
      assert(join && !join->input0.lock() && !join->input1.lock());
      join->input0 = out0;
      join->input1 = out1;
      join->height = std::max(out0->get_height(), out1->get_height()) + 1;
      join->token_owner = out0->get_token_owner();
      join->size = out0->get_size() + out1->get_size();
      join->token_size = out0->get_token_size();

      out0->insert_output_enabled(join);
      if (out0 != out1)
#ifdef ZENI_RETE_LR_UNLINKING
        out1->insert_output_disabled(join);
#else
        out1->insert_output_enabled(join);
      join->data.connected1 = true;
#endif

      out0->pass_tokens(network, join);
#ifndef ZENI_RETE_LR_UNLINKING
      if (out0 != out1)
        out1->pass_tokens(network, join);
#endif
    }

  }

}
