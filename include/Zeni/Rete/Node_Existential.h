#ifndef RETE_EXISTENTIAL_H
#define RETE_EXISTENTIAL_H

#include "Node.h"

namespace Zeni {

  namespace Rete {

    class ZENI_RETE_LINKAGE Rete_Existential : public Node {
      Rete_Existential(const Rete_Existential &);
      Rete_Existential & operator=(const Rete_Existential &);

      friend ZENI_RETE_LINKAGE void bind_to_existential(Network &network, const std::shared_ptr<Node_Existential> &existential, const std::shared_ptr<Node> &out);

    public:
      Rete_Existential();

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

      static std::shared_ptr<Node_Existential> find_existing(const std::shared_ptr<Node> &out);

    private:
      Node * input = nullptr;
      Tokens input_tokens;
      Tokens output_tokens;
      std::shared_ptr<const Token> output_token;
    };

    ZENI_RETE_LINKAGE void bind_to_existential(Network &network, const std::shared_ptr<Node_Existential> &existential, const std::shared_ptr<Node> &out);

  }

}

#endif
