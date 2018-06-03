#ifndef ZENI_RETE_RAVEN_DECREMENT_OUTPUT_COUNT_HPP
#define ZENI_RETE_RAVEN_DECREMENT_OUTPUT_COUNT_HPP

#include "Raven.hpp"

namespace Zeni::Rete {

  class Raven_Decrement_Output_Count : public Rete::Raven {
    Raven_Decrement_Output_Count(const Raven_Decrement_Output_Count &) = delete;
    Raven_Decrement_Output_Count & operator=(const Raven_Decrement_Output_Count &) = delete;

  public:
    ZENI_RETE_LINKAGE Raven_Decrement_Output_Count(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> output);

    void receive() const override;
  };

}

#endif
