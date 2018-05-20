#ifndef RETE_NEGATION_H
#define RETE_NEGATION_H

#include "Node.h"

namespace Zeni {

  namespace Rete {

    class Node_Negation : public Node {
      Node_Negation(const Node_Negation &);
      Node_Negation & operator=(const Node_Negation &);

      friend ZENI_RETE_LINKAGE void bind_to_negation(Network &network, const std::shared_ptr<Node_Negation> &negation, const std::shared_ptr<Node> &out);

    public:
      ZENI_RETE_LINKAGE Node_Negation();

      ZENI_RETE_LINKAGE void destroy(Network &network, const std::shared_ptr<Node> &output) override;

      ZENI_RETE_LINKAGE std::shared_ptr<const Node> parent_left() const override { return input.lock(); }
      ZENI_RETE_LINKAGE std::shared_ptr<const Node> parent_right() const override { return input.lock(); }
      ZENI_RETE_LINKAGE std::shared_ptr<Node> parent_left() override { return input.lock(); }
      ZENI_RETE_LINKAGE std::shared_ptr<Node> parent_right() override { return input.lock(); }

      ZENI_RETE_LINKAGE std::shared_ptr<const Node_Filter> get_filter(const int64_t &index) const override;

      ZENI_RETE_LINKAGE const Tokens & get_output_tokens() const override;
      ZENI_RETE_LINKAGE bool has_output_tokens() const override;

      ZENI_RETE_LINKAGE void insert_token(Network &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &from) override;
      ZENI_RETE_LINKAGE bool remove_token(Network &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &from) override;

      ZENI_RETE_LINKAGE void pass_tokens(Network &network, const std::shared_ptr<Node> &output) override;
      ZENI_RETE_LINKAGE void unpass_tokens(Network &network, const std::shared_ptr<Node> &output) override;

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

    ZENI_RETE_LINKAGE void bind_to_negation(Network &network, const std::shared_ptr<Node_Negation> &negation, const std::shared_ptr<Node> &out);

  }

}

#endif
