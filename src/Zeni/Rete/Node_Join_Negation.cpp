#include "rete_negation_join.h"

#include "rete_action.h"
#include "rete_existential.h"
#include "rete_negation.h"

namespace Rete {

  Rete_Negation_Join::Rete_Negation_Join(WME_Bindings bindings_) : bindings(bindings_) {}

  void Rete_Negation_Join::destroy(Rete_Agent &agent, const Rete_Node_Ptr &output) {
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

  Rete_Filter_Ptr_C Rete_Negation_Join::get_filter(const int64_t &index) const {
    const int64_t left_size = parent_left()->get_token_size();
    if(index < left_size)
      return parent_left()->get_filter(index);
    else
      return parent_right()->get_filter(index - left_size);
  }

  const Rete_Node::Tokens & Rete_Negation_Join::get_output_tokens() const {
    return output_tokens;
  }

  bool Rete_Negation_Join::has_output_tokens() const {
    return !output_tokens.empty();
  }

  void Rete_Negation_Join::insert_wme_token(Rete_Agent &agent, const WME_Token_Ptr_C &wme_token, const Rete_Node * const &from) {
    assert(from == input0 || from == input1);

    if(from == input0) {
      auto &match = matching[std::make_pair(wme_token, true)];
      const auto inserted = match.first.insert(wme_token);
      if(inserted.second) {
        ++input0_count;
        if(match.second.empty())
          join_tokens(agent, *inserted.first);
      }
    }
    if(from == input1) {
      auto &match = matching[std::make_pair(wme_token, false)];
      const auto inserted = match.second.insert(wme_token);
      if(inserted.second) {
        ++input1_count;
        if(match.second.size() == 1u) {
          for(const auto &other : match.first)
            unjoin_tokens(agent, other);
        }
      }
    }
  }

  bool Rete_Negation_Join::remove_wme_token(Rete_Agent &agent, const WME_Token_Ptr_C &wme_token, const Rete_Node * const &from) {
    assert(from == input0 || from == input1);

    bool emptied = false;

    if(from == input0) {
      const auto index = std::make_pair(wme_token, true);
      auto &match = matching[index];
      auto found2 = match.first.find(wme_token);

      if(found2 != match.first.end()) {
        if(match.second.empty())
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
            join_tokens(agent, other);
        }
        match.second.erase(found2);

        emptied ^= !--input1_count;
      }

      if(match.second.empty() && match.first.empty())
        matching.erase(index);
    }

    return emptied;
  }

  bool Rete_Negation_Join::operator==(const Rete_Node &rhs) const {
    if(auto join = dynamic_cast<const Rete_Negation_Join *>(&rhs))
      return bindings == join->bindings && input0 == join->input0 && input1 == join->input1;
    return false;
  }

  void Rete_Negation_Join::print_details(std::ostream &os) const {
    os << "  " << intptr_t(this) << " [label=\"&not;&exist;J\\n";
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

  void Rete_Negation_Join::print_rule(std::ostream &os, const Variable_Indices_Ptr_C &indices, const Rete_Node_Ptr_C &suppress) const {
    if(suppress && this == suppress->parent_left().get()) {
      os << '&' << dynamic_cast<const Rete_Action *>(suppress.get())->get_name();
      return;
    }

    const auto pl = parent_left();
    const auto pr = parent_right();
    const bool prb = !dynamic_cast<const Rete_Filter *>(pr.get());

    pl->print_rule(os, indices, suppress);
    os << std::endl << "  ";

    os << '-';
    if(prb)
      os << '{';

    const auto bound = bind_Variable_Indices(bindings, indices, *pl, *pr);

    pr->print_rule(os, bound, suppress);

    if(prb)
      os << '}';
  }

  void Rete_Negation_Join::output_name(std::ostream &os, const int64_t &depth) const {
    os << "nj(" << bindings << ',';
    if(input0 && depth)
      input0->output_name(os, depth - 1);
    os << ',';
    if(input1 && depth)
      input1->output_name(os, depth - 1);
    os << ')';
  }

  bool Rete_Negation_Join::is_active() const {
    return Rete_Negation_Join::has_output_tokens();
  }

  std::vector<WME> Rete_Negation_Join::get_filter_wmes() const {
    auto filter_wmes0 = input0->get_filter_wmes();
    auto filter_wmes1 = input1->get_filter_wmes();
    filter_wmes0.insert(filter_wmes0.end(), filter_wmes1.begin(), filter_wmes1.end());
    return filter_wmes0;
  }

  Rete_Negation_Join_Ptr Rete_Negation_Join::find_existing(const WME_Bindings &bindings, const Rete_Node_Ptr &out0, const Rete_Node_Ptr &out1) {
    if(get_Option_Ranged<bool>(Options::get_global(), "rete-disable-node-sharing"))
      return nullptr;

    for(auto &o0 : out0->get_outputs_all()) {
      if(auto existing_negation_join = std::dynamic_pointer_cast<Rete_Negation_Join>(o0)) {
        if(std::find(out1->get_outputs_all().begin(), out1->get_outputs_all().end(), existing_negation_join) != out1->get_outputs_all().end()) {
          if(bindings == existing_negation_join->bindings)
            return existing_negation_join;
        }
      }
    }

    return nullptr;
  }

  void Rete_Negation_Join::join_tokens(Rete_Agent &agent, const WME_Token_Ptr_C &lhs) {
//    for(auto &binding : bindings) {
//      if(*(*lhs.first)[binding.first] != *(*rhs)[binding.second])
//        return;
//    }

//    if(--lhs.second == 0) {
    const auto token = output_tokens.insert(lhs);

    for(auto &output : *outputs_enabled)
      output.ptr->insert_wme_token(agent, *token.first, this);
//    }
  }

  void Rete_Negation_Join::unjoin_tokens(Rete_Agent &agent, const WME_Token_Ptr_C &lhs) {
//    for(auto &binding : bindings) {
//      if(*(*lhs.first)[binding.first] != *(*rhs)[binding.second])
//        return;
//    }

//    if(++lhs.second == 1) {
    for(auto ot = outputs_enabled->begin(), oend = outputs_enabled->end(); ot != oend; ) {
      if((*ot)->remove_wme_token(agent, lhs, this))
        (*ot++)->disconnect(agent, this);
      else
        ++ot;
    }

    output_tokens.erase(lhs);
//    }
  }

  void Rete_Negation_Join::pass_tokens(Rete_Agent &agent, Rete_Node * const &output) {
    for(auto &wme_token : output_tokens)
      output->insert_wme_token(agent, wme_token, this);
  }

  void Rete_Negation_Join::unpass_tokens(Rete_Agent &agent, Rete_Node * const &output) {
    for(auto &wme_token : output_tokens)
      output->remove_wme_token(agent, wme_token, this);
  }

  void bind_to_negation_join(Rete_Agent &agent, const Rete_Negation_Join_Ptr &join, const Rete_Node_Ptr &out0, const Rete_Node_Ptr &out1) {
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
    out0->pass_tokens(agent, join.get());
    if(out0 != out1) {
      out1->insert_output_enabled(join);
      out1->pass_tokens(agent, join.get());
    }
  }

}
