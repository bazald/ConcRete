#ifndef RETE_EXISTENTIAL_JOIN_H
#define RETE_EXISTENTIAL_JOIN_H

#include "Node.h"

namespace Zeni {

  namespace Rete {

    class ZENI_RETE_LINKAGE Rete_Existential_Join : public Node {
      Rete_Existential_Join(const Rete_Existential_Join &);
      Rete_Existential_Join & operator=(const Rete_Existential_Join &);

      friend ZENI_RETE_LINKAGE void bind_to_existential_join(Network &network, const std::shared_ptr<Node_Join_Existential> &join, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1);

    public:
      Rete_Existential_Join(Variable_Bindings bindings_);

      void destroy(Network &network, const std::shared_ptr<Node> &output) override;

      std::shared_ptr<const Node> parent_left() const override { return input0->shared(); }
      std::shared_ptr<const Node> parent_right() const override { return input1->shared(); }
      std::shared_ptr<Node> parent_left() override { return input0->shared(); }
      std::shared_ptr<Node> parent_right() override { return input1->shared(); }

      std::shared_ptr<const Node_Filter> get_filter(const int64_t &index) const override;

      const Tokens & get_output_tokens() const override;
      bool has_output_tokens() const override;

      void insert_token(Network &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &from) override;
      bool remove_token(Network &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &from) override;

      bool operator==(const Node &rhs) const override;

      bool disabled_input(const std::shared_ptr<Node> &input) override;

      void print_details(std::ostream &os) const override; ///< Formatted for dot: http://www.graphviz.org/content/dot-language

      void print_rule(std::ostream &os, const std::shared_ptr<const Variable_Indices> &indices, const std::shared_ptr<const Node> &suppress) const override;

      void output_name(std::ostream &os, const int64_t &depth) const override;

      bool is_active() const override;

      std::vector<WME> get_filter_wmes() const override;

      static std::shared_ptr<Node_Join_Existential> find_existing(const Variable_Bindings &bindings, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1);

      virtual const Variable_Bindings * get_bindings() const override { return &bindings; }

    private:
      void join_tokens(Network &network, const std::shared_ptr<const Token> &lhs);
      void unjoin_tokens(Network &network, const std::shared_ptr<const Token> &lhs);

      void disconnect(Network &network, const std::shared_ptr<const Node> &from) override;

      void pass_tokens(Network &network, const std::shared_ptr<Node> &output) override;
      void unpass_tokens(Network &network, const std::shared_ptr<Node> &output) override;

      Variable_Bindings bindings;
      Node * input0 = nullptr;
      Node * input1 = nullptr;
      int64_t input0_count = 0;
      int64_t input1_count = 0;

      std::unordered_map<std::pair<std::shared_ptr<const Token>, bool>, std::pair<Tokens, Tokens>, std::function<size_t(const std::pair<std::shared_ptr<const Token>, bool> &)>, std::function<bool(const std::pair<std::shared_ptr<const Token>, bool> &, const std::pair<std::shared_ptr<const Token>, bool> &)>> matching
        = std::unordered_map<std::pair<std::shared_ptr<const Token>, bool>, std::pair<Tokens, Tokens>, std::function<size_t(const std::pair<std::shared_ptr<const Token>, bool> &)>, std::function<bool(const std::pair<std::shared_ptr<const Token>, bool> &, const std::pair<std::shared_ptr<const Token>, bool> &)>>(0, [this](const std::pair<std::shared_ptr<const Token>, bool> &itoken)->size_t {
        return itoken.first->hash_bindings(itoken.second, this->bindings);
      }, [this](const std::pair<std::shared_ptr<const Token>, bool> &lhs, const std::pair<std::shared_ptr<const Token>, bool> &rhs)->bool {
        return lhs.first->eval_bindings(lhs.second, this->bindings, *rhs.first, rhs.second);
      });
      Tokens output_tokens;

      struct Data {
        Data() : connected0(true), connected1(true) {}

        bool connected0 : 1;
        bool connected1 : 1;
      } data;
    };

    ZENI_RETE_LINKAGE void bind_to_existential_join(Network &network, const std::shared_ptr<Node_Join_Existential> &join, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1);

  }

}

#endif
