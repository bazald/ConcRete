#ifndef ZENI_RETE_NODE_NEGATION_H
#define ZENI_RETE_NODE_NEGATION_H

#include "Node.hpp"

namespace Zeni {

  namespace Rete {

    class Node_Negation : public Node {
      Node_Negation(const Node_Negation &);
      Node_Negation & operator=(const Node_Negation &);

      friend ZENI_RETE_LINKAGE void bind_to_negation(const std::shared_ptr<Network> &network, const std::shared_ptr<Node_Negation> &negation, const std::shared_ptr<Node> &out);

      Node_Negation();

    public:
      ZENI_RETE_LINKAGE static std::shared_ptr<Node_Negation> Create(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &out);

      ZENI_RETE_LINKAGE std::shared_ptr<const Node> parent_left() const override { return input.lock(); }
      ZENI_RETE_LINKAGE std::shared_ptr<const Node> parent_right() const override { return input.lock(); }
      ZENI_RETE_LINKAGE std::shared_ptr<Node> parent_left() override { return input.lock(); }
      ZENI_RETE_LINKAGE std::shared_ptr<Node> parent_right() override { return input.lock(); }

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

      ZENI_RETE_LINKAGE static std::shared_ptr<Node_Negation> find_existing(const std::shared_ptr<Node> &out);

    private:
      std::weak_ptr<Node> input;
      Tokens input_tokens;
      Tokens output_tokens;
      std::shared_ptr<const Token> output_token;
    };

  }

}

#endif
