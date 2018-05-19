#include "Zeni/Rete/Node_Join.h"

#include "Zeni/Rete/Node_Action.h"
//#include "Zeni/Rete/Node_Existential.h"
//#include "Zeni/Rete/Node_Negation.h"

#include <cassert>
//#include <typeinfo>

#define RETE_LR_UNLINKING

namespace Zeni {

  namespace Rete {

    Node_Join::Node_Join(Variable_Bindings bindings_)
      : bindings(bindings_)
    {
    }

    void Node_Join::destroy(Network &network, const std::shared_ptr<Node> &output) {
      erase_output(output);
      if (!destruction_suppressed && outputs_all.empty()) {
        //std::cerr << "Destroying: ";
        //output_name(std::cerr, 3);
        //std::cerr << std::endl;

        const auto input0_locked = input0.lock();
        const auto input1_locked = input1.lock();
        const auto sft = shared_from_this();
        input0_locked->destroy(network, sft);
        if (input0_locked != input1_locked)
          input1_locked->destroy(network, sft);
      }
    }

    std::shared_ptr<const Node_Filter> Node_Join::get_filter(const int64_t &index) const {
      const int64_t left_size = parent_left()->get_token_size();
      if (index < left_size)
        return parent_left()->get_filter(index);
      else
        return parent_right()->get_filter(index - left_size);
    }

    const Node::Tokens & Node_Join::get_output_tokens() const {
      return output_tokens;
    }

    bool Node_Join::has_output_tokens() const {
      return !output_tokens.empty();
    }

    void Node_Join::insert_token(Network &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &from) {
      const auto input0_locked = input0.lock();
      const auto input1_locked = input1.lock();

      assert(from == input0_locked || from == input1_locked);

      if (from == input0_locked) {
        //#ifdef DEBUG_OUTPUT
        //      std::cerr << this << " Joining left: " << *token << std::endl;
        //#endif
#ifdef RETE_LR_UNLINKING
        if (!data.connected1) {
          //#ifdef DEBUG_OUTPUT
          //        std::cerr << this << " Connecting right" << std::endl;
          //#endif
          assert(!input1_count);
          input1_locked->enable_output(network, shared_from_this());
          data.connected1 = true;
        }
#endif

        auto &match = matching[std::make_pair(token, true)];
        const auto inserted = match.first.insert(token);
        if (inserted.second) {
          ++input0_count;
          for (const auto &other : match.second) {
            //          std::cerr << "Savings: " << match.second.size() << " / " << input1_tokens.size() << std::endl;
            join_tokens(network, *inserted.first, other);
          }
        }
        //      for(const auto &other : input1_tokens) {
        //        bool match_success = true;
        //        for(auto &binding : bindings) {
        //          if(*(*token)[binding.first] != *(*other)[binding.second])
        //            match_success = false;
        //        }
        //        if(match_success && find_deref(match.second, other) == match.second.end()) {
        //          std::cerr << "Excluded: " << *other << " from " << *token << " on the basis of " << bindings << std::endl;
        //          std::cerr << "Index:";
        //          for(const auto &symbol : index)
        //            std::cerr << ' ' << symbol << ':' << *symbol;
        //          std::cerr << std::endl;
        //          abort();
        //        }
        //      }
      }
      if (from == input1_locked) {
        //#ifdef DEBUG_OUTPUT
        //      std::cerr << this << " Joining right: " << *token << std::endl;
        //#endif
#ifdef RETE_LR_UNLINKING
        if (!data.connected0) {
          //#ifdef DEBUG_OUTPUT
          //        std::cerr << this << " Connecting left" << std::endl;
          //#endif
          assert(!input0_count);
          input0_locked->enable_output(network, shared_from_this());
          data.connected0 = true;
        }
#endif

        auto &match = matching[std::make_pair(token, false)];
        const auto inserted = match.second.insert(token);
        if (inserted.second) {
          ++input1_count;
          for (const auto &other : match.first) {
            //          std::cerr << "Savings: " << match.first.size() << " / " << input0_tokens.size() << std::endl;
            join_tokens(network, other, *inserted.first);
          }
        }
        //      for(const auto &other : input0_tokens) {
        //        bool match_success = true;
        //        for(auto &binding : bindings) {
        //          if(*(*other)[binding.first] != *(*token)[binding.second])
        //            match_success = false;
        //        }
        //        if(match_success && find_deref(match.first, other) == match.first.end()) {
        //          std::cerr << "Excluded: " << *token << " from " << *other << " on the basis of " << bindings << std::endl;
        //          std::cerr << "Index:";
        //          for(const auto &symbol : index)
        //            std::cerr << ' ' << symbol << ':' << *symbol;
        //          std::cerr << std::endl;
        //          abort();
        //        }
        //      }
      }
    }

    bool Node_Join::remove_token(Network &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &from) {
      const auto input0_locked = input0.lock();
      const auto input1_locked = input1.lock();
      const auto sft = shared_from_this();

      assert(from == input0_locked || from == input1_locked);

      bool emptied = false;

      if (from == input0_locked) {
        const auto index = std::make_pair(token, true);
        auto &match = matching[index];
        auto found2 = match.first.find(token);

        if (found2 != match.first.end()) {
          for (const auto &other : match.second) {
            auto found_output = output_tokens.find(join_tokens(*found2, other));
            if (found_output != output_tokens.end()) {
              for (auto ot = outputs_all.begin(), oend = outputs_all.end(); ot != oend; ) {
                if ((*ot)->remove_token(network, (*found_output), sft))
                  (*ot++)->disconnect(network, sft);
                else
                  ++ot;
              }
              output_tokens.erase(found_output);
            }
          }
          match.first.erase(found2);
          //          for(const auto &other : input1_tokens) {
          //            auto found_output = find_deref(output_tokens, join_tokens(token, other));
          //            if(found_output != output_tokens.end())
          //              abort();
          //          }

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
          for (const auto &other : match.first) {
            auto found_output = output_tokens.find(join_tokens(other, *found2));
            if (found_output != output_tokens.end()) {
              for (auto ot = outputs_all.begin(), oend = outputs_all.end(); ot != oend; ) {
                if ((*ot)->remove_token(network, (*found_output), sft))
                  (*ot++)->disconnect(network, sft);
                else
                  ++ot;
              }
              output_tokens.erase(found_output);
            }
          }
          match.second.erase(found2);
          //          for(const auto &other : input0_tokens) {
          //            auto found_output = find_deref(output_tokens, join_tokens(token, other));
          //            if(found_output != output_tokens.end())
          //              abort();
          //          }

          emptied ^= !--input1_count;
        }

        if (match.second.empty() && match.first.empty())
          matching.erase(index);
      }

      return emptied;
    }

    bool Node_Join::operator==(const Node &rhs) const {
      if (auto join = dynamic_cast<const Node_Join *>(&rhs))
        return bindings == join->bindings && input0.lock() == join->input0.lock() && input1.lock() == join->input1.lock();
      return false;
    }

    bool Node_Join::disabled_input(const std::shared_ptr<Node> &
#ifdef RETE_LR_UNLINKING
      input
#endif
    ) {
#ifdef RETE_LR_UNLINKING
      if (input == input0.lock())
        return !data.connected0;
      else {
        assert(input == input1.lock());
        return !data.connected1;
      }
#else
      return false;
#endif
    }

    void Node_Join::print_details(std::ostream &os) const {
      os << "  " << intptr_t(this) << " [label=\"";
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

    void Node_Join::print_rule(std::ostream &os, const std::shared_ptr<const Variable_Indices> &indices, const std::shared_ptr<const Node> &suppress) const {
      if (suppress && this == suppress->parent_left().get()) {
        os << '&' << std::dynamic_pointer_cast<const Node_Action>(suppress)->get_name();
        return;
      }

      const auto pl = parent_left();
      const auto pr = parent_right();
      const bool prb = !dynamic_cast<const Node_Filter *>(pr.get());

      pl->print_rule(os, indices, suppress);
      os << std::endl << "  ";

      if (prb)
        os << '{';

      const auto bound = bind_Variable_Indices(bindings, indices, *pl, *pr);

      pr->print_rule(os, bound, suppress);

      if (prb)
        os << '}';
    }

    void Node_Join::output_name(std::ostream &os, const int64_t &depth) const {
      os << "j(" << bindings << ',';
      const auto input0_locked = input0.lock();
      if (input0_locked && depth)
        input0_locked->output_name(os, depth - 1);
      os << ',';
      const auto input1_locked = input1.lock();
      if (input1_locked && depth)
        input1_locked->output_name(os, depth - 1);
      os << ')';
    }

    bool Node_Join::is_active() const {
      return !output_tokens.empty();
    }

    std::vector<WME> Node_Join::get_filter_wmes() const {
      auto filter_wmes0 = input0.lock()->get_filter_wmes();
      auto filter_wmes1 = input1.lock()->get_filter_wmes();
      filter_wmes0.insert(filter_wmes0.end(), filter_wmes1.begin(), filter_wmes1.end());
      return filter_wmes0;
    }

    std::shared_ptr<Node_Join> Node_Join::find_existing(const Variable_Bindings &bindings, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1) {
      if (get_Option_Ranged<bool>(Options::get_global(), "rete-disable-node-sharing"))
        return nullptr;

      for (auto &o0 : out0->get_outputs_all()) {
        if (auto existing_join = std::dynamic_pointer_cast<Node_Join>(o0)) {
          if (std::find(out1->get_outputs_all().begin(), out1->get_outputs_all().end(), existing_join) != out1->get_outputs_all().end()) {
            if (bindings == existing_join->bindings)
              return existing_join;
          }
        }
      }

      return nullptr;
    }

    void Node_Join::join_tokens(Network &network, const std::shared_ptr<const Token> &lhs, const std::shared_ptr<const Token> &rhs) {
      //    for(auto &binding : bindings) {
      //      if(*(*lhs)[binding.first] != *(*rhs)[binding.second])
      //        return;
      //    }

      //#ifdef DEBUG_OUTPUT
      //    std::cerr << "Joining " << *lhs << " and " << *rhs << std::endl;
      //#endif

      const auto token = output_tokens.insert(join_tokens(lhs, rhs));

      if (token.second) {
        for (auto &output : outputs_enabled)
          output->insert_token(network, (*token.first), shared_from_this());
      }
    }

    std::shared_ptr<const Token> Node_Join::join_tokens(const std::shared_ptr<const Token> lhs, const std::shared_ptr<const Token> &rhs) const {
      if (rhs->size())
        return std::make_shared<Token>(lhs, rhs);
      else
        return lhs;
    }

    void Node_Join::disconnect(Network &
#ifdef RETE_LR_UNLINKING
      network
#endif
      , const std::shared_ptr<const Node> &
#ifdef RETE_LR_UNLINKING
      from
#endif
    ) {
#ifdef RETE_LR_UNLINKING
      const auto input0_locked = input0.lock();
      const auto input1_locked = input1.lock();
      if (from == input0_locked) {
        //#ifdef DEBUG_OUTPUT
        //      std::cerr << this << " Disconnecting right" << std::endl;
        //#endif
        assert(data.connected0);
        assert(data.connected1);
        assert(!input0_count);
        //#ifndef NDEBUG
        //      std::cerr << input1_tokens.size() << std::endl;
        //#endif
        input1_locked->disable_output(network, shared_from_this());
        matching.clear();
        input1_count = 0;
        //#ifndef NDEBUG
        //      std::cerr << typeid(*input1).name() << " failed to unpass" << input1_tokens.size() << std::endl;
        //      for(const auto &token : input1_tokens) {
        //        std::cerr << *token << std::endl;
        //      }
        //#endif
        data.connected1 = false;
      }
      else {
        //#ifdef DEBUG_OUTPUT
        //      std::cerr << this << " Disconnecting left" << std::endl;
        //#endif
        assert(data.connected0);
        assert(data.connected1);
        assert(!input1_count);
        //#ifndef NDEBUG
        //      std::cerr << input0_tokens.size() << std::endl;
        //#endif
        input0_locked->disable_output(network, shared_from_this());
        matching.clear();
        input0_count = 0;
        //#ifndef NDEBUG
        //      std::cerr << typeid(*input0).name() << " failed to unpass" << input0_tokens.size() << std::endl;
        //      for(const auto &token : input0_tokens) {
        //        std::cerr << *token << std::endl;
        //      }
        //#endif
        data.connected0 = false;
      }
      assert(data.connected0 || data.connected1);
#endif
    }

    void Node_Join::pass_tokens(Network &network, const std::shared_ptr<Node> &output) {
      const auto sft = shared_from_this();
      for (auto &token : output_tokens)
        output->insert_token(network, token, sft);
    }

    void Node_Join::unpass_tokens(Network &network, const std::shared_ptr<Node> &output) {
      const auto sft = shared_from_this();
      for (auto &token : output_tokens)
        output->remove_token(network, token, sft);
    }

    void bind_to_join(Network &network, const std::shared_ptr<Node_Join> &join, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1) {
      assert(join && !join->input0.lock() && !join->input1.lock());
      assert(!std::dynamic_pointer_cast<Node_Existential>(out0));
      assert(!std::dynamic_pointer_cast<Node_Negation>(out0));
      join->input0 = out0;
      join->input1 = out1;
      join->height = std::max(out0->get_height(), out1->get_height()) + 1;
      join->token_owner = join;
      join->size = out0->get_size() + out1->get_size();
      join->token_size = out0->get_token_size() + out1->get_token_size();

#ifdef RETE_LR_UNLINKING
      if (!out1->has_output_tokens()) {
        out0->insert_output_enabled(join);

        if (out0 != out1) {
          out1->insert_output_disabled(join);
          join->data.connected1 = false;
        }

        out0->pass_tokens(network, join);
      }
      else {
        out1->insert_output_enabled(join);

        if (out0->has_output_tokens()) {
          if (out0 != out1) {
            out0->insert_output_enabled(join);
            out0->pass_tokens(network, join);
          }
        }
        else {
          out0->insert_output_disabled(join);
          join->data.connected0 = false;
        }

        out1->pass_tokens(network, join);
      }
#else
      out0->insert_output_enabled(join);
      if (out0 != out1)
        out1->insert_output_enabled(join);
      join->data.connected1 = true;

      out0->pass_tokens(network, join);
      if (out0 != out1)
        out1->pass_tokens(network, join);
#endif
    }

  }

}
