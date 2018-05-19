#include "rete_negation.h"

#include "rete_action.h"

namespace Rete {

  Rete_Negation::Rete_Negation()
    : output_token(std::make_shared<WME_Token>())
  {
    output_tokens.insert(output_token);
  }

  void Rete_Negation::destroy(Rete_Agent &agent, const Rete_Node_Ptr &output) {
    erase_output(output);
    if(!destruction_suppressed && outputs_all.empty()) {
      //std::cerr << "Destroying: ";
      //output_name(std::cerr, 3);
      //std::cerr << std::endl;

      input->destroy(agent, shared());
    }
  }

  Rete_Filter_Ptr_C Rete_Negation::get_filter(const int64_t &index) const {
    return parent_left()->get_filter(index);
  }

  const Rete_Node::Tokens & Rete_Negation::get_output_tokens() const {
    return output_tokens;
  }

  bool Rete_Negation::has_output_tokens() const {
    return input_tokens.empty();
  }

  void Rete_Negation::insert_wme_token(Rete_Agent &agent, const WME_Token_Ptr_C &wme_token, const Rete_Node * const &
#ifndef NDEBUG
                                                                                                                     from
#endif
                                                                                                                         ) {
    assert(from == input);

    input_tokens.insert(wme_token);

    if(input_tokens.size() == 1) {
      for(auto ot = outputs_enabled->begin(), oend = outputs_enabled->end(); ot != oend; ) {
        if((*ot)->remove_wme_token(agent, output_token, this))
          (*ot++)->disconnect(agent, this);
        else
          ++ot;
      }
      output_tokens.clear();
    }
  }

  bool Rete_Negation::remove_wme_token(Rete_Agent &agent, const WME_Token_Ptr_C &wme_token, const Rete_Node * const &
#ifndef NDEBUG
                                                                                                                     from
#endif
                                                                                                                        ) {
    assert(from == input);

    auto found = input_tokens.find(wme_token);
    if(found != input_tokens.end()) {
      input_tokens.erase(found);
      if(input_tokens.empty()) {
        output_tokens.insert(output_token);
        for(auto &output : *outputs_enabled)
          output.ptr->insert_wme_token(agent, output_token, this);
      }
    }

    return input_tokens.empty();
  }

  void Rete_Negation::pass_tokens(Rete_Agent &agent, Rete_Node * const &output) {
    if(input_tokens.empty())
      output->insert_wme_token(agent, output_token, this);
  }

  void Rete_Negation::unpass_tokens(Rete_Agent &agent, Rete_Node * const &output) {
    if(input_tokens.empty())
      output->remove_wme_token(agent, output_token, this);
  }

  bool Rete_Negation::operator==(const Rete_Node &rhs) const {
    if(auto negation = dynamic_cast<const Rete_Negation *>(&rhs))
      return input == negation->input;
    return false;
  }

  void Rete_Negation::print_details(std::ostream &os) const {
    os << "  " << intptr_t(this) << " [label=\"&not;&exist;\"];" << std::endl;
    os << "  " << intptr_t(input) << " -> " << intptr_t(this) << " [color=red];" << std::endl;
  }

  void Rete_Negation::print_rule(std::ostream &os, const Variable_Indices_Ptr_C &indices, const Rete_Node_Ptr_C &suppress) const {
    if(suppress && this == suppress->parent_left().get()) {
      os << '&' << dynamic_cast<const Rete_Action *>(suppress.get())->get_name();
      return;
    }

    os << '-';
    const bool pb = get_token_size() > 1;
    if(pb)
      os << '}';

    parent_right()->print_rule(os, indices, suppress);

    if(pb)
      os << '}';
  }

  void Rete_Negation::output_name(std::ostream &os, const int64_t &depth) const {
    os << "n(";
    if(input && depth)
      input->output_name(os, depth - 1);
    os << ')';
  }

  bool Rete_Negation::is_active() const {
    return input_tokens.empty();
  }

  std::vector<WME> Rete_Negation::get_filter_wmes() const {
    return input->get_filter_wmes();
  }

  Rete_Negation_Ptr Rete_Negation::find_existing(const Rete_Node_Ptr &out) {
    if(get_Option_Ranged<bool>(Options::get_global(), "rete-disable-node-sharing"))
      return nullptr;

    for(auto &o : out->get_outputs_all()) {
      if(auto existing_negation = std::dynamic_pointer_cast<Rete_Negation>(o))
        return existing_negation;
    }

    return nullptr;
  }

  void bind_to_negation(Rete_Agent &agent, const Rete_Negation_Ptr &negation, const Rete_Node_Ptr &out) {
    assert(negation);
    negation->input = out.get();
    negation->height = out->get_height() + 1;
    negation->token_owner = out->get_token_owner();
    negation->size = out->get_size();
    negation->token_size = out->get_token_size();
    out->insert_output_enabled(negation);
    out->pass_tokens(agent, negation.get());
  }

}
