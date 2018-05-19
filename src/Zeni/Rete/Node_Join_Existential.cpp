#include "rete_existential_join.h"

#include "rete_action.h"
#include "rete_existential.h"
#include "rete_negation.h"

#undef RETE_LR_UNLINKING

namespace Rete {

  Rete_Existential_Join::Rete_Existential_Join(WME_Bindings bindings_)
   : bindings(bindings_)
  {
  }

  void Rete_Existential_Join::destroy(Rete_Agent &agent, const Rete_Node_Ptr &output) {
    erase_output(output);
    if(!destruction_suppressed && outputs_all.empty()) {
      //std::cerr << "Destroying: ";
      //output_name(std::cerr, 3);
      //std::cerr << std::endl;

      auto i0 = input0->shared();
      auto i1 = input1->shared();
      auto o = shared();
      i0->destroy(agent, o);
      if(i0 != i1)
        i1->destroy(agent, o);
    }
  }

  Rete_Filter_Ptr_C Rete_Existential_Join::get_filter(const int64_t &index) const {
    const int64_t left_size = parent_left()->get_token_size();
    if(index < left_size)
      return parent_left()->get_filter(index);
    else
      return parent_right()->get_filter(index - left_size);
  }

  const Rete_Node::Tokens & Rete_Existential_Join::get_output_tokens() const {
    return output_tokens;
  }

  bool Rete_Existential_Join::has_output_tokens() const {
    return !output_tokens.empty();
  }

  void Rete_Existential_Join::insert_wme_token(Rete_Agent &agent, const WME_Token_Ptr_C &wme_token, const Rete_Node * const &from) {
    assert(from == input0 || from == input1);

    if(from == input0) {
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
        if(!match.second.empty())
          join_tokens(agent, *inserted.first);
      }
    }
    if(from == input1) {
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
        if(match.second.size() == 1u) {
          for(const auto &other : match.first)
            join_tokens(agent, other);
        }
      }
    }
  }

  bool Rete_Existential_Join::remove_wme_token(Rete_Agent &agent, const WME_Token_Ptr_C &wme_token, const Rete_Node * const &from) {
    assert(from == input0 || from == input1);

    bool emptied = false;

    if(from == input0) {
      const auto index = std::make_pair(wme_token, true);
      auto &match = matching[index];
      auto found2 = match.first.find(wme_token);

      if(found2 != match.first.end()) {
        if(!match.second.empty())
          unjoin_tokens(agent, *found2);
        match.first.erase(found2);

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
        if(match.second.size() == 1) {
          for(auto &other : match.first)
            unjoin_tokens(agent, other);
        }
        match.second.erase(found2);

        emptied ^= !--input1_count;
      }

      if(match.second.empty() && match.first.empty())
        matching.erase(index);
    }

    return emptied;
  }

  bool Rete_Existential_Join::operator==(const Rete_Node &rhs) const {
    if(auto join = dynamic_cast<const Rete_Existential_Join *>(&rhs))
      return bindings == join->bindings && input0 == join->input0 && input1 == join->input1;
    return false;
  }

  bool Rete_Existential_Join::disabled_input(const Rete_Node_Ptr &
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

  void Rete_Existential_Join::print_details(std::ostream &os) const {
    os << "  " << intptr_t(this) << " [label=\"&exist;J\\n";
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

  void Rete_Existential_Join::print_rule(std::ostream &os, const Variable_Indices_Ptr_C &indices, const Rete_Node_Ptr_C &suppress) const {
    if(suppress && this == suppress->parent_left().get()) {
      os << '&' << dynamic_cast<const Rete_Action *>(suppress.get())->get_name();
      return;
    }

    const auto pl = parent_left();
    const auto pr = parent_right();
    const bool prb = !dynamic_cast<const Rete_Filter *>(pr.get());

    pl->print_rule(os, indices, suppress);
    os << std::endl << "  ";

    os << '+';
    if(prb)
      os << '{';

    const auto bound = bind_Variable_Indices(bindings, indices, *pl, *pr);

    pr->print_rule(os, bound, suppress);

    if(prb)
      os << '}';
  }

  void Rete_Existential_Join::output_name(std::ostream &os, const int64_t &depth) const {
    os << "ej(" << bindings << ',';
    if(input0 && depth)
      input0->output_name(os, depth - 1);
    os << ',';
    if(input1 && depth)
      input1->output_name(os, depth - 1);
    os << ')';
  }

  bool Rete_Existential_Join::is_active() const {
    return Rete_Existential_Join::has_output_tokens();
  }

  std::vector<WME> Rete_Existential_Join::get_filter_wmes() const {
    auto filter_wmes0 = input0->get_filter_wmes();
    auto filter_wmes1 = input1->get_filter_wmes();
    filter_wmes0.insert(filter_wmes0.end(), filter_wmes1.begin(), filter_wmes1.end());
    return filter_wmes0;
  }

  Rete_Existential_Join_Ptr Rete_Existential_Join::find_existing(const WME_Bindings &bindings, const Rete_Node_Ptr &out0, const Rete_Node_Ptr &out1) {
    if(get_Option_Ranged<bool>(Options::get_global(), "rete-disable-node-sharing"))
      return nullptr;

    for(auto &o0 : out0->get_outputs_all()) {
      if(auto existing_existential_join = std::dynamic_pointer_cast<Rete_Existential_Join>(o0)) {
        if(std::find(out1->get_outputs_all().begin(), out1->get_outputs_all().end(), existing_existential_join) != out1->get_outputs_all().end()) {
          if(bindings == existing_existential_join->bindings)
            return existing_existential_join;
        }
      }
    }

    return nullptr;
  }

  void Rete_Existential_Join::join_tokens(Rete_Agent &agent, const WME_Token_Ptr_C &lhs) {
//    for(auto &binding : bindings) {
//      if(*(*lhs.first)[binding.first] != *(*rhs)[binding.second])
//        return;
//    }

//    if(++lhs.second == 1u) {
    const auto token = output_tokens.insert(lhs);

    for(auto &output : *outputs_enabled)
      output.ptr->insert_wme_token(agent, *token.first, this);
//    }
  }

  void Rete_Existential_Join::unjoin_tokens(Rete_Agent &agent, const WME_Token_Ptr_C &lhs) {
//    for(auto &binding : bindings) {
//      if(*(*lhs.first)[binding.first] != *(*rhs)[binding.second])
//        return;
//    }

//    if(--lhs.second == 0) {
    for(auto ot = outputs_enabled->begin(), oend = outputs_enabled->end(); ot != oend; ) {
      if((*ot)->remove_wme_token(agent, lhs, this))
        (*ot++)->disconnect(agent, this);
      else
        ++ot;
    }

    output_tokens.erase(lhs);
//    }
  }

  void Rete_Existential_Join::pass_tokens(Rete_Agent &agent, Rete_Node * const &output) {
    for(auto &wme_token : output_tokens)
      output->insert_wme_token(agent, wme_token, this);
  }

  void Rete_Existential_Join::unpass_tokens(Rete_Agent &agent, Rete_Node * const &output) {
    for(auto &wme_token : output_tokens)
      output->remove_wme_token(agent, wme_token, this);
  }

  void Rete_Existential_Join::disconnect(Rete_Agent &
#ifdef RETE_LR_UNLINKING
                                                     agent
#endif
                                                          , const Rete_Node * const &
#ifdef RETE_LR_UNLINKING
                                                                                     from
#endif
                                                                                         ) {
#ifdef RETE_LR_UNLINKING
    if(input0 != input1) {
      if(from == input0) {
        assert(data.connected1);
        input1->disable_output(agent, this);
        data.connected1 = false;
      }
      else {
        assert(data.connected0);
        input0->disable_output(agent, this);
        data.connected0 = false;
      }
    }
    assert(data.connected0 || data.connected1);
#endif
  }

  void bind_to_existential_join(Rete_Agent &agent, const Rete_Existential_Join_Ptr &join, const Rete_Node_Ptr &out0, const Rete_Node_Ptr &out1) {
    assert(join && !join->input0 && !join->input1);
    assert(!std::dynamic_pointer_cast<Rete_Existential>(out0));
    assert(!std::dynamic_pointer_cast<Rete_Negation>(out0));
    join->input0 = out0.get();
    join->input1 = out1.get();
    join->height = std::max(out0->get_height(), out1->get_height()) + 1;
    join->token_owner = out0->get_token_owner();
    join->size = out0->get_size() + out1->get_size();
    join->token_size = out0->get_token_size();

    out0->insert_output_enabled(join);
    if(out0 != out1)
#ifdef RETE_LR_UNLINKING
      out1->insert_output_disabled(join);
#else
      out1->insert_output_enabled(join);
    join->data.connected1 = true;
#endif

    out0->pass_tokens(agent, join.get());
#ifndef RETE_LR_UNLINKING
    if(out0 != out1)
      out1->pass_tokens(agent, join.get());
#endif
  }

}
