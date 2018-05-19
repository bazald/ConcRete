#ifndef RETE_FILTER_H
#define RETE_FILTER_H

#include "Node.h"

namespace Zeni {

  namespace Rete {

    class ZENI_RETE_LINKAGE Node_Filter : public Node {
      Node_Filter(const Node_Filter &);
      Node_Filter & operator=(const Node_Filter &);

    public:
      enum Index { LEFT = 0, CENTER = 1, RIGHT = 2 };

      Node_Filter(const WME &wme_);

      const WME & get_wme() const;

      void destroy(Network &network, const std::shared_ptr<Node> &output) override;

      std::shared_ptr<const Node> parent_left() const override { abort(); }
      std::shared_ptr<const Node> parent_right() const override { abort(); }
      std::shared_ptr<Node> parent_left() override { abort(); }
      std::shared_ptr<Node> parent_right() override { abort(); }

      std::shared_ptr<const Node_Filter> get_filter(const int64_t &index) const override;

      const Tokens & get_output_tokens() const override;
      bool has_output_tokens() const override;

      void insert_wme(Network &network, const std::shared_ptr<const WME> &wme);
      void remove_wme(Network &network, const std::shared_ptr<const WME> &wme);

      void insert_token(Network &network, const std::shared_ptr<const Token> &, const std::shared_ptr<const Node> &) override;
      bool remove_token(Network &network, const std::shared_ptr<const Token> &, const std::shared_ptr<const Node> &) override;

      void pass_tokens(Network &network, const std::shared_ptr<Node> &output) override;
      void unpass_tokens(Network &network, const std::shared_ptr<Node> &output) override;

      bool operator==(const Node &rhs) const override;

      void print_details(std::ostream &os) const override; ///< Formatted for dot: http://www.graphviz.org/content/dot-language

      void print_rule(std::ostream &os, const std::shared_ptr<const Variable_Indices> &indices, const std::shared_ptr<const Node> &suppress) const override;

      void output_name(std::ostream &os, const int64_t &depth) const override;

      bool is_active() const override;

      std::vector<WME> get_filter_wmes() const override;

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
