#ifndef ZENI_RETE_NODE_FILTER_H
#define ZENI_RETE_NODE_FILTER_H

#include "Node.hpp"

namespace Zeni {

  namespace Rete {

    class Node_Filter : public Node {
      Node_Filter(const Node_Filter &) = delete;
      Node_Filter & operator=(const Node_Filter &) = delete;

    public:
      enum Index { LEFT = 0, CENTER = 1, RIGHT = 2 };
      
    private:
      Node_Filter(const WME &wme_);

    public:
      ZENI_RETE_LINKAGE static std::shared_ptr<Node_Filter> Create_Or_Increment_Output_Count(const std::shared_ptr<Network> &network, const WME &wme);

      ZENI_RETE_LINKAGE const WME & get_wme() const;

      ZENI_RETE_LINKAGE void send_disconnect_from_parents(const std::shared_ptr<Network> &network) override;

      ZENI_RETE_LINKAGE bool receive(const Raven_Token_Insert &raven) override;
      ZENI_RETE_LINKAGE bool receive(const Raven_Token_Remove &raven) override;

      ZENI_RETE_LINKAGE bool operator==(const Node &rhs) const override;

    private:
      WME m_wme;
      std::tuple<std::shared_ptr<const Symbol_Variable>, std::shared_ptr<const Symbol_Variable>, std::shared_ptr<const Symbol_Variable>> m_variable;

      Tokens m_output_antitokens;
    };

  }

}

#define ZENI_RETE_FILTER_H_DONE
#endif
