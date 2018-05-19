#include "rete_join.h"

#include "rete_action.h"
#include "rete_existential.h"
#include "rete_negation.h"

#include <typeinfo>

#define RETE_LR_UNLINKING

namespace Rete {

  Rete_Join::Rete_Join(WME_Bindings bindings_)
   : bindings(bindings_)
  {
  }

  void Rete_Join::destroy(Rete_Agent &agent, const Rete_Node_Ptr &output) {
    erase_output(output);
    if(!destruction_suppressed && outputs_all.empty()) {
      //std::cerr << "Destroying: ";
      //output_name(std::cerr, 3);
      //std::cerr << std::endl;

      auto i0 = input0;
      auto i1 = input1;
      auto o = shared();
      i0->destroy(agent, o);
      if(i0 != i1)
        i1->destroy(agent, o);
    }
  }

  Rete_Filter_Ptr_C Rete_Join::get_filter(const int64_t &index) const {
    const int64_t left_size = parent_left()->get_token_size();
    if(index < left_size)
      return parent_left()->get_filter(index);
    else
      return parent_right()->get_filter(index - left_size);
  }

  const Rete_Node::Tokens & Rete_Join::get_output_tokens() const {
    return output_tokens;
  }

  bool Rete_Join::has_output_tokens() const {
    return !output_tokens.empty();
  }

  void Rete_Join::insert_wme_token(Rete_Agent &agent, const WME_Token_Ptr_C &wme_token, const Rete_Node * const &from) {
    assert(from == input0 || from == input1);

    if(from == input0) {
//#ifdef DEBUG_OUTPUT
//      std::cerr << this << " Joining left: " << *wme_token << std::endl;
//#endif
#ifdef RETE_LR_UNLINKING
      if(!data.connected1) {
//#ifdef DEBUG_OUTPUT
//        std::cerr << this << " Connecting right" << std::endl;
//#endif
        assert(!input1_count);
        input1->enable_output(agent, this);
        data.connected1 = true;
      }
#endif

      auto &match = matching[std::make_pair(wme_token, true)];
      const auto inserted = match.first.insert(wme_token);
      if(inserted.second) {
        ++input0_count;
        for(const auto &other : match.second) {
//          std::cerr << "Savings: " << match.second.size() << " / " << input1_tokens.size() << std::endl;
          join_tokens(agent, *inserted.first, other);
        }
      }
//      for(const auto &other : input1_tokens) {
//        bool match_success = true;
//        for(auto &binding : bindings) {
//          if(*(*wme_token)[binding.first] != *(*other)[binding.second])
//            match_success = false;
//        }
//        if(match_success && find_deref(match.second, other) == match.second.end()) {
//          std::cerr << "Excluded: " << *other << " from " << *wme_token << " on the basis of " << bindings << std::endl;
//          std::cerr << "Index:";
//          for(const auto &symbol : index)
//            std::cerr << ' ' << symbol << ':' << *symbol;
//          std::cerr << std::endl;
//          abort();
//        }
//      }
    }
    if(from == input1) {
//#ifdef DEBUG_OUTPUT
//      std::cerr << this << " Joining right: " << *wme_token << std::endl;
//#endif
#ifdef RETE_LR_UNLINKING
      if(!data.connected0) {
//#ifdef DEBUG_OUTPUT
//        std::cerr << this << " Connecting left" << std::endl;
//#endif
        assert(!input0_count);
        input0->enable_output(agent, this);
        data.connected0 = true;
      }
#endif

      auto &match = matching[std::make_pair(wme_token, false)];
      const auto inserted = match.second.insert(wme_token);
      if(inserted.second) {
        ++input1_count;
        for(const auto &other : match.first) {
//          std::cerr << "Savings: " << match.first.size() << " / " << input0_tokens.size() << std::endl;
          join_tokens(agent, other, *inserted.first);
        }
      }
//      for(const auto &other : input0_tokens) {
//        bool match_success = true;
//        for(auto &binding : bindings) {
//          if(*(*other)[binding.first] != *(*wme_token)[binding.second])
//            match_success = false;
//        }
//        if(match_success && find_deref(match.first, other) == match.first.end()) {
//          std::cerr << "Excluded: " << *wme_token << " from " << *other << " on the basis of " << bindings << std::endl;
//          std::cerr << "Index:";
//          for(const auto &symbol : index)
//            std::cerr << ' ' << symbol << ':' << *symbol;
//          std::cerr << std::endl;
//          abort();
//        }
//      }
    }
  }

  bool Rete_Join::remove_wme_token(Rete_Agent &agent, const WME_Token_Ptr_C &wme_token, const Rete_Node * const &from) {
    assert(from == input0 || from == input1);

    bool emptied = false;

    if(from == input0) {
      const auto index = std::make_pair(wme_token, true);
      auto &match = matching[index];
      auto found2 = match.first.find(wme_token);

      if(found2 != match.first.end()) {
        for(const auto &other : match.second) {
          auto found_output = output_tokens.find(join_wme_tokens(*found2, other));
          if(found_output != output_tokens.end()) {
            for(auto ot = outputs_all.begin(), oend = outputs_all.end(); ot != oend; ) {
              if((*ot)->remove_wme_token(agent, (*found_output), this))
                (*ot++)->disconnect(agent, this);
              else
                ++ot;
            }
            output_tokens.erase(found_output);
          }
        }
        match.first.erase(found2);
//          for(const auto &other : input1_tokens) {
//            auto found_output = find_deref(output_tokens, join_wme_tokens(wme_token, other));
//            if(found_output != output_tokens.end())
//              abort();
//          }

        emptied ^= !--input0_count;
      }

      if(match.first.empty() && match.second.empty())
        matching.erase(index);
    }
    if(from == input1) {
      const auto index = std::make_pair(wme_token, false);
      auto &match = matching[index];
      auto found2 = match.second.find(wme_token);

      if(found2 != match.second.end()) {
        for(const auto &other : match.first) {
          auto found_output = output_tokens.find(join_wme_tokens(other, *found2));
          if(found_output != output_tokens.end()) {
            for(auto ot = outputs_all.begin(), oend = outputs_all.end(); ot != oend; ) {
              if((*ot)->remove_wme_token(agent, (*found_output), this))
                (*ot++)->disconnect(agent, this);
              else
                ++ot;
            }
            output_tokens.erase(found_output);
          }
        }
        match.second.erase(found2);
//          for(const auto &other : input0_tokens) {
//            auto found_output = find_deref(output_tokens, join_wme_tokens(wme_token, other));
//            if(found_output != output_tokens.end())
//              abort();
//          }

        emptied ^= !--input1_count;
      }

      if(match.second.empty() && match.first.empty())
        matching.erase(index);
    }

    return emptied;
  }

  bool Rete_Join::operator==(const Rete_Node &rhs) const {
    if(auto join = dynamic_cast<const Rete_Join *>(&rhs))
      return bindings == join->bindings && input0 == join->input0 && input1 == join->input1;
    return false;
  }

  bool Rete_Join::disabled_input(const Rete_Node_Ptr &
#ifdef RETE_LR_UNLINKING
                                                      input
#endif
                                                           ) {
#ifdef RETE_LR_UNLINKING
    if(input.get() == input0)
      return !data.connected0;
    else {
      assert(input.get() == input1);
      return !data.connected1;
    }
#else
    return false;
#endif
  }

  void Rete_Join::print_details(std::ostream &os) const {
    os << "  " << intptr_t(this) << " [label=\"";
    if(bindings.empty())
      os << "&empty;";
    else {
      auto bt = bindings.begin();
      os << *bt++;
      while(bt != bindings.end())
        os << "\\n" << *bt++;
    }
    os << "\"];" << std::endl;

    os << "  " << intptr_t(input0) << " -> " << intptr_t(this) << " [color=red];" << std::endl;
    os << "  " << intptr_t(input1) << " -> " << intptr_t(this) << " [color=blue];" << std::endl;
  }

  void Rete_Join::print_rule(std::ostream &os, const Variable_Indices_Ptr_C &indices, const Rete_Node_Ptr_C &suppress) const {
    if(suppress && this == suppress->parent_left().get()) {
      os << '&' << dynamic_cast<const Rete_Action *>(suppress.get())->get_name();
      return;
    }

    const auto pl = parent_left();
    const auto pr = parent_right();
    const bool prb = !dynamic_cast<const Rete_Filter *>(pr.get());

    pl->print_rule(os, indices, suppress);
    os << std::endl << "  ";

    if(prb)
      os << '{';

    const auto bound = bind_Variable_Indices(bindings, indices, *pl, *pr);

    pr->print_rule(os, bound, suppress);

    if(prb)
      os << '}';
  }

  void Rete_Join::output_name(std::ostream &os, const int64_t &depth) const {
    os << "j(" << bindings << ',';
    if(input0 && depth)
      input0->output_name(os, depth - 1);
    os << ',';
    if(input1 && depth)
      input1->output_name(os, depth - 1);
    os << ')';
  }

  bool Rete_Join::is_active() const {
    return !output_tokens.empty();
  }

  std::vector<WME> Rete_Join::get_filter_wmes() const {
    auto filter_wmes0 = input0->get_filter_wmes();
    auto filter_wmes1 = input1->get_filter_wmes();
    filter_wmes0.insert(filter_wmes0.end(), filter_wmes1.begin(), filter_wmes1.end());
    return filter_wmes0;
  }

  Rete_Join_Ptr Rete_Join::find_existing(const WME_Bindings &bindings, const Rete_Node_Ptr &out0, const Rete_Node_Ptr &out1) {
    if(get_Option_Ranged<bool>(Options::get_global(), "rete-disable-node-sharing"))
      return nullptr;

    for(auto &o0 : out0->get_outputs_all()) {
      if(auto existing_join = std::dynamic_pointer_cast<Rete_Join>(o0)) {
        if(std::find(out1->get_outputs_all().begin(), out1->get_outputs_all().end(), existing_join) != out1->get_outputs_all().end()) {
          if(bindings == existing_join->bindings)
            return existing_join;
        }
      }
    }

    return nullptr;
  }

  void Rete_Join::join_tokens(Rete_Agent &agent, const WME_Token_Ptr_C &lhs, const WME_Token_Ptr_C &rhs) {
//    for(auto &binding : bindings) {
//      if(*(*lhs)[binding.first] != *(*rhs)[binding.second])
//        return;
//    }

//#ifdef DEBUG_OUTPUT
//    std::cerr << "Joining " << *lhs << " and " << *rhs << std::endl;
//#endif

    const auto token = output_tokens.insert(join_wme_tokens(lhs, rhs));

    if(token.second) {
      for(auto &output : *outputs_enabled)
        output.ptr->insert_wme_token(agent, (*token.first), this);
    }
  }

  WME_Token_Ptr_C Rete_Join::join_wme_tokens(const WME_Token_Ptr_C lhs, const WME_Token_Ptr_C &rhs) const {
    if(rhs->size())
      return std::make_shared<WME_Token>(lhs, rhs);
    else
      return lhs;
  }

  void Rete_Join::disconnect(Rete_Agent &
#ifdef RETE_LR_UNLINKING
                                         agent
#endif
                                              , const Rete_Node * const &
#ifdef RETE_LR_UNLINKING
                                                                         from
#endif
                                                                             ) {
#ifdef RETE_LR_UNLINKING
    assert(input0 != input1);
    if(from == input0) {
//#ifdef DEBUG_OUTPUT
//      std::cerr << this << " Disconnecting right" << std::endl;
//#endif
      assert(data.connected0);
      assert(data.connected1);
      assert(!input0_count);
//#ifndef NDEBUG
//      std::cerr << input1_tokens.size() << std::endl;
//#endif
      input1->disable_output(agent, this);
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
      input0->disable_output(agent, this);
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

  void Rete_Join::pass_tokens(Rete_Agent &agent, Rete_Node * const &output) {
    for(auto &wme_token : output_tokens)
      output->insert_wme_token(agent, wme_token, this);
  }

  void Rete_Join::unpass_tokens(Rete_Agent &agent, Rete_Node * const &output) {
    for(auto &wme_token : output_tokens)
      output->remove_wme_token(agent, wme_token, this);
  }

  void bind_to_join(Rete_Agent &agent, const Rete_Join_Ptr &join, const Rete_Node_Ptr &out0, const Rete_Node_Ptr &out1) {
    assert(join && !join->input0 && !join->input1);
    assert(!std::dynamic_pointer_cast<Rete_Existential>(out0));
    assert(!std::dynamic_pointer_cast<Rete_Negation>(out0));
    join->input0 = out0.get();
    join->input1 = out1.get();
    join->height = std::max(out0->get_height(), out1->get_height()) + 1;
    join->token_owner = join;
    join->size = out0->get_size() + out1->get_size();
    join->token_size = out0->get_token_size() + out1->get_token_size();

#ifdef RETE_LR_UNLINKING
    if(!out1->has_output_tokens()) {
      out0->insert_output_enabled(join);

      if(out0 != out1) {
        out1->insert_output_disabled(join);
        join->data.connected1 = false;
      }

      out0->pass_tokens(agent, join.get());
    }
    else {
      out1->insert_output_enabled(join);

      if(out0->has_output_tokens()) {
        if(out0 != out1) {
          out0->insert_output_enabled(join);
          out0->pass_tokens(agent, join.get());
        }
      }
      else {
        out0->insert_output_disabled(join);
        join->data.connected0 = false;
      }

      out1->pass_tokens(agent, join.get());
    }
#else
    out0->insert_output_enabled(join);
    if(out0 != out1)
      out1->insert_output_enabled(join);
    join->data.connected1 = true;

    out0->pass_tokens(agent, join.get());
    if(out0 != out1)
      out1->pass_tokens(agent, join.get());
#endif
  }

}
