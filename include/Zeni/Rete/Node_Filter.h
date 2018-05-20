#ifndef RETE_FILTER_H
#define RETE_FILTER_H

#include "Node.h"

namespace Zeni {

  namespace Rete {

    class Node_Filter : public Node {
      Node_Filter(const Node_Filter &);
      Node_Filter & operator=(const Node_Filter &);

    public:
      enum Index { LEFT = 0, CENTER = 1, RIGHT = 2 };

      ZENI_RETE_LINKAGE Node_Filter(const WME &wme_);

      ZENI_RETE_LINKAGE const WME & get_wme() const;

      ZENI_RETE_LINKAGE void destroy(Network &network, const std::shared_ptr<Node> &output) override;

      ZENI_RETE_LINKAGE std::shared_ptr<const Node> parent_left() const override { abort(); }
      ZENI_RETE_LINKAGE std::shared_ptr<const Node> parent_right() const override { abort(); }
      ZENI_RETE_LINKAGE std::shared_ptr<Node> parent_left() override { abort(); }
      ZENI_RETE_LINKAGE std::shared_ptr<Node> parent_right() override { abort(); }

      ZENI_RETE_LINKAGE std::shared_ptr<const Node_Filter> get_filter(const int64_t &index) const override;

      ZENI_RETE_LINKAGE const Tokens & get_output_tokens() const override;
      ZENI_RETE_LINKAGE bool has_output_tokens() const override;

      ZENI_RETE_LINKAGE void insert_wme(Network &network, const std::shared_ptr<const WME> &wme);
      ZENI_RETE_LINKAGE void remove_wme(Network &network, const std::shared_ptr<const WME> &wme);

      ZENI_RETE_LINKAGE void insert_token(Network &network, const std::shared_ptr<const Token> &, const std::shared_ptr<const Node> &) override;
      ZENI_RETE_LINKAGE bool remove_token(Network &network, const std::shared_ptr<const Token> &, const std::shared_ptr<const Node> &) override;

      ZENI_RETE_LINKAGE void pass_tokens(Network &network, const std::shared_ptr<Node> &output) override;
      ZENI_RETE_LINKAGE void unpass_tokens(Network &network, const std::shared_ptr<Node> &output) override;

      ZENI_RETE_LINKAGE bool operator==(const Node &rhs) const override;

      ZENI_RETE_LINKAGE void print_details(std::ostream &os) const override; ///< Formatted for dot: http://www.graphviz.org/content/dot-language

      ZENI_RETE_LINKAGE void print_rule(std::ostream &os, const std::shared_ptr<const Variable_Indices> &indices, const std::shared_ptr<const Node> &suppress) const override;

      ZENI_RETE_LINKAGE void output_name(std::ostream &os, const int64_t &depth) const override;

      ZENI_RETE_LINKAGE bool is_active() const override;

      ZENI_RETE_LINKAGE std::vector<WME> get_filter_wmes() const override;

    private:
      WME m_wme;
      std::array<std::shared_ptr<const Symbol_Variable>, 3> m_variable;
      Tokens tokens;
    };

    ZENI_RETE_LINKAGE void bind_to_filter(Network &network, const std::shared_ptr<Node_Filter> &filter);

  }

}

#define RETE_FILTER_H_DONE
#endif
