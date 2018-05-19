#ifndef RETE_PREDICATE_H
#define RETE_PREDICATE_H

#include "Node.h"

namespace Zeni {

  namespace Rete {

    class ZENI_RETE_LINKAGE Rete_Predicate : public Node {
      Rete_Predicate(const Rete_Predicate &);
      Rete_Predicate & operator=(const Rete_Predicate &);

      friend ZENI_RETE_LINKAGE void bind_to_predicate(Network &network, const std::shared_ptr<Node_Predicate> &predicate, const std::shared_ptr<Node> &out);

    public:
      enum Predicate { EQ, NEQ, GT, GTE, LT, LTE };

      Rete_Predicate(const Predicate &predicate_, const Token_Index lhs_index_, const Token_Index rhs_index_);
      Rete_Predicate(const Predicate &predicate_, const Token_Index lhs_index_, const std::shared_ptr<const Symbol> &rhs_);

      void destroy(Network &network, const std::shared_ptr<Node> &output) override;

      std::shared_ptr<const Node> parent_left() const override { return input->shared(); }
      std::shared_ptr<const Node> parent_right() const override { return input->shared(); }
      std::shared_ptr<Node> parent_left() override { return input->shared(); }
      std::shared_ptr<Node> parent_right() override { return input->shared(); }

      std::shared_ptr<const Node_Filter> get_filter(const int64_t &index) const override;

      const Tokens & get_output_tokens() const override;
      bool has_output_tokens() const override;

      void insert_token(Network &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &from) override;
      bool remove_token(Network &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &from) override;

      void pass_tokens(Network &network, const std::shared_ptr<Node> &output) override;
      void unpass_tokens(Network &network, const std::shared_ptr<Node> &output) override;

      bool operator==(const Node &rhs) const override;

      void print_details(std::ostream &os) const override; ///< Formatted for dot: http://www.graphviz.org/content/dot-language

      void print_rule(std::ostream &os, const std::shared_ptr<const Variable_Indices> &indices, const std::shared_ptr<const Node> &suppress) const override;

      void output_name(std::ostream &os, const int64_t &depth) const override;

      bool is_active() const override;

      std::vector<WME> get_filter_wmes() const override;

      static std::shared_ptr<Node_Predicate> find_existing(const Predicate &predicate, const Token_Index &lhs_index, const Token_Index &rhs_index, const std::shared_ptr<Node> &out);
      static std::shared_ptr<Node_Predicate> find_existing(const Predicate &predicate, const Token_Index &lhs_index, const std::shared_ptr<const Symbol> &rhs, const std::shared_ptr<Node> &out);

      const Token_Index & get_lhs_index() const { return m_lhs_index; }
      const Predicate & get_predicate() const { return m_predicate; }
      std::string get_predicate_str() const;
      const Token_Index & get_rhs_index() const { return m_rhs_index; }
      const std::shared_ptr<const Symbol> & get_rhs() const { return m_rhs; }

    private:
      bool test_predicate(const std::shared_ptr<const Symbol> &lhs, const std::shared_ptr<const Symbol> &rhs) const;

      Predicate m_predicate;
      Token_Index m_lhs_index;
      Token_Index m_rhs_index;
      std::shared_ptr<const Symbol> m_rhs;
      Node * input = nullptr;
      Tokens tokens;
    };

    ZENI_RETE_LINKAGE void bind_to_predicate(Network &network, const std::shared_ptr<Node_Predicate> &predicate, const std::shared_ptr<Node> &out);

  }

}

#endif
