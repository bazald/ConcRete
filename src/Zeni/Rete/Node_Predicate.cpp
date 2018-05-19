#include "rete_predicate.h"

#include "rete_action.h"
#include "rete_existential.h"
#include "rete_negation.h"

#include <sstream>

namespace Rete {

  Rete_Predicate::Rete_Predicate(const Predicate &predicate_, const WME_Token_Index lhs_index_, const WME_Token_Index rhs_index_)
   : m_predicate(predicate_),
   m_lhs_index(lhs_index_),
   m_rhs_index(rhs_index_)
  {
    assert(m_lhs_index.rete_row >= m_lhs_index.token_row);
    assert(m_rhs_index.rete_row >= m_rhs_index.token_row);
  }

  Rete_Predicate::Rete_Predicate(const Predicate &predicate_, const WME_Token_Index lhs_index_, const Symbol_Ptr_C &rhs_)
   : m_predicate(predicate_),
   m_lhs_index(lhs_index_),
   m_rhs(rhs_)
  {
    assert(m_lhs_index.rete_row >= m_lhs_index.token_row);
  }

  void Rete_Predicate::destroy(Rete_Agent &agent, const Rete_Node_Ptr &output) {
    erase_output(output);
    if(!destruction_suppressed && outputs_all.empty()) {
      //std::cerr << "Destroying: ";
      //output_name(std::cerr, 3);
      //std::cerr << std::endl;

      input->destroy(agent, shared());
    }
  }

  Rete_Filter_Ptr_C Rete_Predicate::get_filter(const int64_t &index) const {
    return parent_left()->get_filter(index);
  }

  const Rete_Node::Tokens & Rete_Predicate::get_output_tokens() const {
    return tokens;
  }

  bool Rete_Predicate::has_output_tokens() const {
    return !tokens.empty();
  }

  void Rete_Predicate::insert_wme_token(Rete_Agent &agent, const WME_Token_Ptr_C &wme_token, const Rete_Node * const &
#ifndef NDEBUG
                                                                                                                      from
#endif
                                                                                                                          ) {
    assert(from == input);

    if(m_rhs) {
      if(!test_predicate((*wme_token)[m_lhs_index], m_rhs))
        return;
    }
    else {
      if(!test_predicate((*wme_token)[m_lhs_index], (*wme_token)[m_rhs_index]))
        return;
    }

    const auto inserted = tokens.insert(wme_token);
    if(inserted.second) {
      for(auto &output : *outputs_enabled)
        output.ptr->insert_wme_token(agent, *inserted.first, this);
    }
  }

  bool Rete_Predicate::remove_wme_token(Rete_Agent &agent, const WME_Token_Ptr_C &wme_token, const Rete_Node * const &
#ifndef NDEBUG
                                                                                                                      from
#endif
                                                                                                                          ) {
    assert(from == input);

    auto found = tokens.find(wme_token);
    if(found != tokens.end()) {
      for(auto ot = outputs_enabled->begin(), oend = outputs_enabled->end(); ot != oend; ) {
        if((*ot)->remove_wme_token(agent, *found, this))
          (*ot++)->disconnect(agent, this);
        else
          ++ot;
      }
      tokens.erase(found);
    }

    return tokens.empty();
  }

  void Rete_Predicate::pass_tokens(Rete_Agent &agent, Rete_Node * const &output) {
    for(auto &wme_token : tokens)
      output->insert_wme_token(agent, wme_token, this);
  }

  void Rete_Predicate::unpass_tokens(Rete_Agent &agent, Rete_Node * const &output) {
//#ifndef NDEBUG
//    std::cerr << "Unpassing " << tokens.size() << " Rete_Predicate tokens." << std::endl;
//#endif
    for(auto &wme_token : tokens)
      output->remove_wme_token(agent, wme_token, this);
  }

  bool Rete_Predicate::operator==(const Rete_Node &rhs) const {
    if(auto predicate = dynamic_cast<const Rete_Predicate *>(&rhs)) {
      return m_predicate == predicate->m_predicate &&
             m_lhs_index == predicate->m_rhs_index &&
             m_rhs_index == predicate->m_rhs_index &&
             *m_rhs == *predicate->m_rhs &&
             input == predicate->input;
    }
    return false;
  }

  void Rete_Predicate::print_details(std::ostream &os) const {
    os << "  " << intptr_t(this) << " [label=\"" << m_lhs_index;
    switch(m_predicate) {
      case EQ: os << '='; break;
      case NEQ: os << "&ne;"; break;
      case GT: os << '>'; break;
      case GTE: os << "&ge;"; break;
      case LT: os << '<'; break;
      case LTE: os << "&le;"; break;
      default: abort();
    }
    if(m_rhs)
      os << *m_rhs;
    else
      os << m_rhs_index;
    os << "\"];" << std::endl;

    os << "  " << intptr_t(input) << " -> " << intptr_t(this) << " [color=red];" << std::endl;
  }

  void Rete_Predicate::print_rule(std::ostream &os, const Variable_Indices_Ptr_C &indices, const Rete_Node_Ptr_C &suppress) const {
    if(suppress && this == suppress->parent_left().get()) {
      os << '&' << dynamic_cast<const Rete_Action *>(suppress.get())->get_name();
      return;
    }

    parent_left()->print_rule(os, indices, suppress);
    os << std::endl << "  ";

    os << "(<" << get_Variable_name(indices, m_lhs_index) << "> ";

    switch(m_predicate) {
      case EQ: os << "=="; break;
      case NEQ: os << "!="; break;
      case GT: os << '>'; break;
      case GTE: os << ">="; break;
      case LT: os << '<'; break;
      case LTE: os << "<="; break;
      default: abort();
    }
    os << ' ';

    if(m_rhs)
      os << *m_rhs;
    else
      os << '<' << get_Variable_name(indices, m_rhs_index) << '>';

    os << ')';
  }

  void Rete_Predicate::output_name(std::ostream &os, const int64_t &depth) const {
    switch(m_predicate) {
      case EQ: os << "EQ"; break;
      case NEQ: os << "NEQ"; break;
      case GT: os << "GT"; break;
      case GTE: os << "GTE"; break;
      case LT: os << "LT"; break;
      case LTE: os << "LTE"; break;
      default: abort();
    }
    os << '(' << m_lhs_index << ',';
    if(m_rhs)
      os << *m_rhs;
    else
      os << m_rhs_index;
    os << ',';
    if(input && depth)
      input->output_name(os, depth - 1);
    os << ')';
  }

  bool Rete_Predicate::is_active() const {
    return !tokens.empty();
  }

  std::vector<WME> Rete_Predicate::get_filter_wmes() const {
    return input->get_filter_wmes();
  }

  Rete_Predicate_Ptr Rete_Predicate::find_existing(const Predicate &predicate, const WME_Token_Index &lhs_index, const WME_Token_Index &rhs_index, const Rete_Node_Ptr &out) {
    for(auto &o : out->get_outputs_all()) {
      if(auto existing_predicate = std::dynamic_pointer_cast<Rete_Predicate>(o)) {
        if(predicate == existing_predicate->m_predicate &&
           lhs_index == existing_predicate->m_lhs_index &&
           rhs_index == existing_predicate->m_rhs_index)
        {
          return existing_predicate;
        }
      }
    }

    return nullptr;
  }

  Rete_Predicate_Ptr Rete_Predicate::find_existing(const Predicate &predicate, const WME_Token_Index &lhs_index, const Symbol_Ptr_C &rhs, const Rete_Node_Ptr &out) {
    if(get_Option_Ranged<bool>(Options::get_global(), "rete-disable-node-sharing"))
      return nullptr;

    for(auto &o : out->get_outputs_all()) {
      if(auto existing_predicate = std::dynamic_pointer_cast<Rete_Predicate>(o)) {
        if(predicate == existing_predicate->m_predicate &&
           lhs_index == existing_predicate->m_lhs_index &&
           *rhs == *existing_predicate->m_rhs)
        {
          return existing_predicate;
        }
      }
    }

    return nullptr;
  }

  std::string Rete_Predicate::get_predicate_str() const {
    switch(m_predicate) {
      case EQ: return "EQ";
      case NEQ: return "NEQ";
      case GT: return "GT";
      case GTE: return "GTE";
      case LT: return "LT";
      case LTE: return "LTE";
      default: abort();
    }
  }

  bool Rete_Predicate::test_predicate(const Symbol_Ptr_C &lhs, const Symbol_Ptr_C &rhs) const {
    switch(m_predicate) {
      case EQ: return *lhs == *rhs;
      case NEQ: return *lhs != *rhs;
      case GT: return *lhs > *rhs;
      case GTE: return *lhs >= *rhs;
      case LT: return *lhs < *rhs;
      case LTE: return *lhs <= *rhs;
      default: abort();
    }
  }

  void bind_to_predicate(Rete_Agent &agent, const Rete_Predicate_Ptr &predicate, const Rete_Node_Ptr &out) {
    assert(predicate);
    assert(!std::dynamic_pointer_cast<Rete_Existential>(out));
    assert(!std::dynamic_pointer_cast<Rete_Negation>(out));
    predicate->input = out.get();
    predicate->height = out->get_height() + 1;
    predicate->token_owner = out->get_token_owner();
    predicate->size = out->get_size() + 1;
    predicate->token_size = out->get_token_size();

    out->insert_output_enabled(predicate);
    out->pass_tokens(agent, predicate.get());
  }

}
