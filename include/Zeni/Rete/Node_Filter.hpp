#ifndef ZENI_RETE_NODE_FILTER_H
#define ZENI_RETE_NODE_FILTER_H

#include "Node_Unary.hpp"

namespace Zeni {

  namespace Rete {

    class Node_Filter : public Node_Unary {
      Node_Filter(const Node_Filter &) = delete;
      Node_Filter & operator=(const Node_Filter &) = delete;

    public:
      enum Index { LEFT = 0, CENTER = 1, RIGHT = 2 };

    private:
      Node_Filter(const std::shared_ptr<Network> &network, const WME &wme_);

    public:
      ZENI_RETE_LINKAGE static std::shared_ptr<Node_Filter> Create_Or_Increment_Output_Count(const std::shared_ptr<Network> &network, const WME &wme);

      ZENI_RETE_LINKAGE const WME & get_wme() const;

      ZENI_RETE_LINKAGE bool receive(const Raven_Token_Insert &raven) override;
      ZENI_RETE_LINKAGE bool receive(const Raven_Token_Remove &raven) override;

      ZENI_RETE_LINKAGE bool operator==(const Node &rhs) const override;

    private:
      const WME m_wme;
      const std::tuple<std::shared_ptr<const Symbol_Variable>, std::shared_ptr<const Symbol_Variable>, std::shared_ptr<const Symbol_Variable>> m_variable;
    };

  }

}

#define ZENI_RETE_FILTER_H_DONE
#endif
