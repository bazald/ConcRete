#ifndef ZENI_RETE_NODE_FILTER_HPP
#define ZENI_RETE_NODE_FILTER_HPP

#include "Internal/Node_Unary.hpp"

namespace Zeni::Rete {

  class Node_Filter : public Node_Unary {
    Node_Filter(const Node_Filter &) = delete;
    Node_Filter & operator=(const Node_Filter &) = delete;

    Node_Filter(const std::shared_ptr<Network> network, const WME wme_);

  public:
    ZENI_RETE_LINKAGE ~Node_Filter();

    ZENI_RETE_LINKAGE static std::shared_ptr<Node_Filter> Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const WME &wme);

    ZENI_RETE_LINKAGE const WME & get_wme() const;

    ZENI_RETE_LINKAGE void receive(const Message_Token_Insert &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Token_Remove &message) override;

    ZENI_RETE_LINKAGE bool operator==(const Node &rhs) const override;

  private:
    const WME m_wme;
    const std::tuple<std::shared_ptr<const Symbol_Variable>, std::shared_ptr<const Symbol_Variable>, std::shared_ptr<const Symbol_Variable>> m_variable;
  };

}

#endif
