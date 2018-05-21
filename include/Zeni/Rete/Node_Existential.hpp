#ifndef ZENI_RETE_NODE_EXISTENTIAL_H
#define ZENI_RETE_NODE_EXISTENTIAL_H

#include "Node.hpp"

namespace Zeni {

  namespace Rete {

    class Node_Existential : public Node {
      Node_Existential(const Node_Existential &);
      Node_Existential & operator=(const Node_Existential &);

      friend ZENI_RETE_LINKAGE void bind_to_existential(const std::shared_ptr<Network> &network, const std::shared_ptr<Node_Existential> &existential, const std::shared_ptr<Node> &out);

      Node_Existential();

    public:
      ZENI_RETE_LINKAGE static std::shared_ptr<Node_Existential> Create();

      ZENI_RETE_LINKAGE void Destroy(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output) override;

      ZENI_RETE_LINKAGE std::shared_ptr<const Node> parent_left() const override;
      ZENI_RETE_LINKAGE std::shared_ptr<const Node> parent_right() const override;
      ZENI_RETE_LINKAGE std::shared_ptr<Node> parent_left() override;
      ZENI_RETE_LINKAGE std::shared_ptr<Node> parent_right() override;

      ZENI_RETE_LINKAGE std::shared_ptr<const Node_Filter> get_filter(const int64_t &index) const override;

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

      ZENI_RETE_LINKAGE static std::shared_ptr<Node_Existential> find_existing(const std::shared_ptr<Node> &out);

    private:
      std::weak_ptr<Node> input;
      Tokens input_tokens;
      Tokens output_tokens;
      std::shared_ptr<const Token> output_token;
    };

    ZENI_RETE_LINKAGE void bind_to_existential(const std::shared_ptr<Network> &network, const std::shared_ptr<Node_Existential> &existential, const std::shared_ptr<Node> &out);

  }

}

#endif
