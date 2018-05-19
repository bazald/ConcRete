#include "Zeni/Rete/Node_Predicate.h"

#include "Zeni/Rete/Node_Action.h"
//#include "Zeni/Rete/Node_Existential.h"
//#include "Zeni/Rete/Node_Negation.h"

#include <cassert>
#include <sstream>

namespace Zeni {

  namespace Rete {

    Node_Predicate::Node_Predicate(const Predicate &predicate_, const Token_Index lhs_index_, const Token_Index rhs_index_)
      : m_predicate(predicate_),
      m_lhs_index(lhs_index_),
      m_rhs_index(rhs_index_)
    {
      assert(m_lhs_index.rete_row >= m_lhs_index.token_row);
      assert(m_rhs_index.rete_row >= m_rhs_index.token_row);
    }

    Node_Predicate::Node_Predicate(const Predicate &predicate_, const Token_Index lhs_index_, const std::shared_ptr<const Symbol> &rhs_)
      : m_predicate(predicate_),
      m_lhs_index(lhs_index_),
      m_rhs(rhs_)
    {
      assert(m_lhs_index.rete_row >= m_lhs_index.token_row);
    }

    void Node_Predicate::destroy(Network &network, const std::shared_ptr<Node> &output) {
      erase_output(output);
      if (!destruction_suppressed && outputs_all.empty()) {
        //std::cerr << "Destroying: ";
        //output_name(std::cerr, 3);
        //std::cerr << std::endl;

        input.lock()->destroy(network, shared_from_this());
      }
    }

    std::shared_ptr<const Node> Node_Predicate::parent_left() const { return input.lock(); }
    std::shared_ptr<const Node> Node_Predicate::parent_right() const { return input.lock(); }
    std::shared_ptr<Node> Node_Predicate::parent_left() { return input.lock(); }
    std::shared_ptr<Node> Node_Predicate::parent_right() { return input.lock(); }

    std::shared_ptr<const Node_Filter> Node_Predicate::get_filter(const int64_t &index) const {
      return parent_left()->get_filter(index);
    }

    const Node::Tokens & Node_Predicate::get_output_tokens() const {
      return tokens;
    }

    bool Node_Predicate::has_output_tokens() const {
      return !tokens.empty();
    }

    void Node_Predicate::insert_token(Network &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &
#ifndef NDEBUG
      from
#endif
    ) {
      assert(from == input.lock());

      if (m_rhs) {
        if (!test_predicate((*token)[m_lhs_index], m_rhs))
          return;
      }
      else {
        if (!test_predicate((*token)[m_lhs_index], (*token)[m_rhs_index]))
          return;
      }

      const auto inserted = tokens.insert(token);
      if (inserted.second) {
        for (auto &output : outputs_enabled)
          output->insert_token(network, *inserted.first, shared_from_this());
      }
    }

    bool Node_Predicate::remove_token(Network &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &
#ifndef NDEBUG
      from
#endif
    ) {
      assert(from == input.lock());

      auto found = tokens.find(token);
      if (found != tokens.end()) {
        for (auto ot = outputs_enabled.begin(), oend = outputs_enabled.end(); ot != oend; ) {
          if ((*ot)->remove_token(network, *found, shared_from_this()))
            (*ot++)->disconnect(network, shared_from_this());
          else
            ++ot;
        }
        tokens.erase(found);
      }

      return tokens.empty();
    }

    void Node_Predicate::pass_tokens(Network &network, const std::shared_ptr<Node> &output) {
      for (auto &token : tokens)
        output->insert_token(network, token, shared_from_this());
    }

    void Node_Predicate::unpass_tokens(Network &network, const std::shared_ptr<Node> &output) {
      //#ifndef NDEBUG
      //    std::cerr << "Unpassing " << tokens.size() << " Node_Predicate tokens." << std::endl;
      //#endif
      for (auto &token : tokens)
        output->remove_token(network, token, shared_from_this());
    }

    bool Node_Predicate::operator==(const Node &rhs) const {
      if (auto predicate = dynamic_cast<const Node_Predicate *>(&rhs)) {
        return m_predicate == predicate->m_predicate &&
          m_lhs_index == predicate->m_rhs_index &&
          m_rhs_index == predicate->m_rhs_index &&
          *m_rhs == *predicate->m_rhs &&
          input.lock() == predicate->input.lock();
      }
      return false;
    }

    void Node_Predicate::print_details(std::ostream &os) const {
      os << "  " << intptr_t(this) << " [label=\"" << m_lhs_index;
      switch (m_predicate) {
      case EQ: os << '='; break;
      case NEQ: os << "&ne;"; break;
      case GT: os << '>'; break;
      case GTE: os << "&ge;"; break;
      case LT: os << '<'; break;
      case LTE: os << "&le;"; break;
      default: abort();
      }
      if (m_rhs)
        os << *m_rhs;
      else
        os << m_rhs_index;
      os << "\"];" << std::endl;

      os << "  " << intptr_t(input.lock().get()) << " -> " << intptr_t(this) << " [color=red];" << std::endl;
    }

    void Node_Predicate::print_rule(std::ostream &os, const std::shared_ptr<const Variable_Indices> &indices, const std::shared_ptr<const Node> &suppress) const {
      if (suppress && this == suppress->parent_left().get()) {
        os << '&' << std::dynamic_pointer_cast<const Node_Action>(suppress)->get_name();
        return;
      }

      parent_left()->print_rule(os, indices, suppress);
      os << std::endl << "  ";

      os << "(<" << get_Variable_name(indices, m_lhs_index) << "> ";

      switch (m_predicate) {
      case EQ: os << "=="; break;
      case NEQ: os << "!="; break;
      case GT: os << '>'; break;
      case GTE: os << ">="; break;
      case LT: os << '<'; break;
      case LTE: os << "<="; break;
      default: abort();
      }
      os << ' ';

      if (m_rhs)
        os << *m_rhs;
      else
        os << '<' << get_Variable_name(indices, m_rhs_index) << '>';

      os << ')';
    }

    void Node_Predicate::output_name(std::ostream &os, const int64_t &depth) const {
      switch (m_predicate) {
      case EQ: os << "EQ"; break;
      case NEQ: os << "NEQ"; break;
      case GT: os << "GT"; break;
      case GTE: os << "GTE"; break;
      case LT: os << "LT"; break;
      case LTE: os << "LTE"; break;
      default: abort();
      }
      os << '(' << m_lhs_index << ',';
      if (m_rhs)
        os << *m_rhs;
      else
        os << m_rhs_index;
      os << ',';
      const auto input_locked = input.lock();
      if (input_locked && depth)
        input_locked->output_name(os, depth - 1);
      os << ')';
    }

    bool Node_Predicate::is_active() const {
      return !tokens.empty();
    }

    std::vector<WME> Node_Predicate::get_filter_wmes() const {
      return input.lock()->get_filter_wmes();
    }

    std::shared_ptr<Node_Predicate> Node_Predicate::find_existing(const Predicate &predicate, const Token_Index &lhs_index, const Token_Index &rhs_index, const std::shared_ptr<Node> &out) {
      for (auto &o : out->get_outputs_all()) {
        if (auto existing_predicate = std::dynamic_pointer_cast<Node_Predicate>(o)) {
          if (predicate == existing_predicate->m_predicate &&
            lhs_index == existing_predicate->m_lhs_index &&
            rhs_index == existing_predicate->m_rhs_index)
          {
            return existing_predicate;
          }
        }
      }

      return nullptr;
    }

    std::shared_ptr<Node_Predicate> Node_Predicate::find_existing(const Predicate &predicate, const Token_Index &lhs_index, const std::shared_ptr<const Symbol> &rhs, const std::shared_ptr<Node> &out) {
      if (get_Option_Ranged<bool>(Options::get_global(), "rete-disable-node-sharing"))
        return nullptr;

      for (auto &o : out->get_outputs_all()) {
        if (auto existing_predicate = std::dynamic_pointer_cast<Node_Predicate>(o)) {
          if (predicate == existing_predicate->m_predicate &&
            lhs_index == existing_predicate->m_lhs_index &&
            *rhs == *existing_predicate->m_rhs)
          {
            return existing_predicate;
          }
        }
      }

      return nullptr;
    }

    std::string Node_Predicate::get_predicate_str() const {
      switch (m_predicate) {
      case EQ: return "EQ";
      case NEQ: return "NEQ";
      case GT: return "GT";
      case GTE: return "GTE";
      case LT: return "LT";
      case LTE: return "LTE";
      default: abort();
      }
    }

    bool Node_Predicate::test_predicate(const std::shared_ptr<const Symbol> &lhs, const std::shared_ptr<const Symbol> &rhs) const {
      switch (m_predicate) {
      case EQ: return *lhs == *rhs;
      case NEQ: return *lhs != *rhs;
      case GT: return *lhs > *rhs;
      case GTE: return *lhs >= *rhs;
      case LT: return *lhs < *rhs;
      case LTE: return *lhs <= *rhs;
      default: abort();
      }
    }

    void bind_to_predicate(Network &network, const std::shared_ptr<Node_Predicate> &predicate, const std::shared_ptr<Node> &out) {
      assert(predicate);
      assert(!std::dynamic_pointer_cast<Node_Existential>(out));
      assert(!std::dynamic_pointer_cast<Node_Negation>(out));
      predicate->input = out;
      predicate->height = out->get_height() + 1;
      predicate->token_owner = out->get_token_owner();
      predicate->size = out->get_size() + 1;
      predicate->token_size = out->get_token_size();

      out->insert_output_enabled(predicate);
      out->pass_tokens(network, predicate);
    }

  }

}
