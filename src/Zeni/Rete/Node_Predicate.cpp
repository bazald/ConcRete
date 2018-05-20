#include "Zeni/Rete/Node_Predicate.hpp"

#include "Zeni/Rete/Node_Action.hpp"

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
        const auto sft = shared_from_this();
        for (auto &output : outputs_enabled)
          output->insert_token(network, *inserted.first, sft);
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
        const auto sft = shared_from_this();
        for (auto ot = outputs_enabled.begin(), oend = outputs_enabled.end(); ot != oend; ) {
          if ((*ot)->remove_token(network, *found, sft))
            (*ot++)->disconnect(network, sft);
          else
            ++ot;
        }
        tokens.erase(found);
      }

      return tokens.empty();
    }

    void Node_Predicate::pass_tokens(Network &network, const std::shared_ptr<Node> &output) {
      const auto sft = shared_from_this();
      for (auto &token : tokens)
        output->insert_token(network, token, sft);
    }

    void Node_Predicate::unpass_tokens(Network &network, const std::shared_ptr<Node> &output) {
      //#ifndef NDEBUG
      //    std::cerr << "Unpassing " << tokens.size() << " Node_Predicate tokens." << std::endl;
      //#endif
      const auto sft = shared_from_this();
      for (auto &token : tokens)
        output->remove_token(network, token, sft);
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
      case Predicate::EQ: os << '='; break;
      case Predicate::NEQ: os << "&ne;"; break;
      case Predicate::GT: os << '>'; break;
      case Predicate::GTE: os << "&ge;"; break;
      case Predicate::LT: os << '<'; break;
      case Predicate::LTE: os << "&le;"; break;
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
      case Predicate::EQ: os << "=="; break;
      case Predicate::NEQ: os << "!="; break;
      case Predicate::GT: os << '>'; break;
      case Predicate::GTE: os << ">="; break;
      case Predicate::LT: os << '<'; break;
      case Predicate::LTE: os << "<="; break;
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
      case Predicate::EQ: os << "EQ"; break;
      case Predicate::NEQ: os << "NEQ"; break;
      case Predicate::GT: os << "GT"; break;
      case Predicate::GTE: os << "GTE"; break;
      case Predicate::LT: os << "LT"; break;
      case Predicate::LTE: os << "LTE"; break;
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
      case Predicate::EQ: return "EQ";
      case Predicate::NEQ: return "NEQ";
      case Predicate::GT: return "GT";
      case Predicate::GTE: return "GTE";
      case Predicate::LT: return "LT";
      case Predicate::LTE: return "LTE";
      default: abort();
      }
    }

    bool Node_Predicate::test_predicate(const std::shared_ptr<const Symbol> &lhs, const std::shared_ptr<const Symbol> &rhs) const {
      switch (m_predicate) {
      case Predicate::EQ: return *lhs == *rhs;
      case Predicate::NEQ: return *lhs != *rhs;
      case Predicate::GT: return *lhs > *rhs;
      case Predicate::GTE: return *lhs >= *rhs;
      case Predicate::LT: return *lhs < *rhs;
      case Predicate::LTE: return *lhs <= *rhs;
      default: abort();
      }
    }

    void bind_to_predicate(Network &network, const std::shared_ptr<Node_Predicate> &predicate, const std::shared_ptr<Node> &out) {
      assert(predicate);
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
