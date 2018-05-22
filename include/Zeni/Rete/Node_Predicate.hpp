#ifndef ZENI_RETE_NODE_PREDICATE_H
#define ZENI_RETE_NODE_PREDICATE_H

#include "Node.hpp"

namespace Zeni {

  namespace Rete {

    class Node_Predicate : public Node {
      Node_Predicate(const Node_Predicate &);
      Node_Predicate & operator=(const Node_Predicate &);

      friend ZENI_RETE_LINKAGE void bind_to_predicate(const std::shared_ptr<Network> &network, const std::shared_ptr<Node_Predicate> &predicate, const std::shared_ptr<Node> &out);

    public:
      enum class ZENI_RETE_LINKAGE Predicate { EQ, NEQ, GT, GTE, LT, LTE };

    private:
      ZENI_RETE_LINKAGE Node_Predicate(const Predicate &predicate_, const Token_Index lhs_index_, const Token_Index rhs_index_);
      ZENI_RETE_LINKAGE Node_Predicate(const Predicate &predicate_, const Token_Index lhs_index_, const std::shared_ptr<const Symbol> &rhs_);

    public:
      ZENI_RETE_LINKAGE static std::shared_ptr<Node_Predicate> Create(const std::shared_ptr<Network> &network, const Node_Predicate::Predicate &pred, const Token_Index &lhs_index, const std::shared_ptr<const Symbol> &rhs, const std::shared_ptr<Node> &out);
      ZENI_RETE_LINKAGE static std::shared_ptr<Node_Predicate> Create(const std::shared_ptr<Network> &network, const Node_Predicate::Predicate &pred, const Token_Index &lhs_index, const Token_Index &rhs_index, const std::shared_ptr<Node> &out);

      ZENI_RETE_LINKAGE std::shared_ptr<const Node> parent_left() const override;
      ZENI_RETE_LINKAGE std::shared_ptr<const Node> parent_right() const override;
      ZENI_RETE_LINKAGE std::shared_ptr<Node> parent_left() override;
      ZENI_RETE_LINKAGE std::shared_ptr<Node> parent_right() override;

      ZENI_RETE_LINKAGE const Tokens & get_output_tokens() const override;
      ZENI_RETE_LINKAGE bool has_output_tokens() const override;

      ZENI_RETE_LINKAGE void insert_token(const std::shared_ptr<Network> &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &from) override;
      ZENI_RETE_LINKAGE void remove_token(const std::shared_ptr<Network> &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &from) override;

      ZENI_RETE_LINKAGE void pass_tokens(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output) override;
      ZENI_RETE_LINKAGE void unpass_tokens(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output) override;

      ZENI_RETE_LINKAGE bool operator==(const Node &rhs) const override;

      ZENI_RETE_LINKAGE void print_details(std::ostream &os) const override; ///< Formatted for dot: http://www.graphviz.org/content/dot-language

      ZENI_RETE_LINKAGE void print_rule(std::ostream &os, const std::shared_ptr<const Variable_Indices> &indices, const std::shared_ptr<const Node> &suppress) const override;

      ZENI_RETE_LINKAGE void output_name(std::ostream &os, const int64_t &depth) const override;

      ZENI_RETE_LINKAGE bool is_active() const override;

      ZENI_RETE_LINKAGE std::vector<WME> get_filter_wmes() const override;

      ZENI_RETE_LINKAGE static std::shared_ptr<Node_Predicate> find_existing(const Predicate &predicate, const Token_Index &lhs_index, const Token_Index &rhs_index, const std::shared_ptr<Node> &out);
      ZENI_RETE_LINKAGE static std::shared_ptr<Node_Predicate> find_existing(const Predicate &predicate, const Token_Index &lhs_index, const std::shared_ptr<const Symbol> &rhs, const std::shared_ptr<Node> &out);

      ZENI_RETE_LINKAGE const Token_Index & get_lhs_index() const { return m_lhs_index; }
      ZENI_RETE_LINKAGE const Predicate & get_predicate() const { return m_predicate; }
      ZENI_RETE_LINKAGE std::string get_predicate_str() const;
      ZENI_RETE_LINKAGE const Token_Index & get_rhs_index() const { return m_rhs_index; }
      ZENI_RETE_LINKAGE const std::shared_ptr<const Symbol> & get_rhs() const { return m_rhs; }

    private:
      bool test_predicate(const std::shared_ptr<const Symbol> &lhs, const std::shared_ptr<const Symbol> &rhs) const;

      Predicate m_predicate;
      Token_Index m_lhs_index;
      Token_Index m_rhs_index;
      std::shared_ptr<const Symbol> m_rhs;
      std::weak_ptr<Node> input;
      Tokens tokens;
    };

  }

}

#endif
