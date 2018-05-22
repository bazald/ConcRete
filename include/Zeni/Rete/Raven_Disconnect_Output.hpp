#ifndef ZENI_RETE_RAVEN_DISCONNECT_OUTPUT_H
#define ZENI_RETE_RAVEN_DISCONNECT_OUTPUT_H

#include "Zeni/Concurrency/Raven.hpp"
#include "Zeni/Rete/Node.hpp"

namespace Zeni {

  namespace Rete {

    class Network;
    class Node_Filter;
    class Token;

    class Raven_Disconnect_Output : public Concurrency::Raven {
      Raven_Disconnect_Output(const Raven_Disconnect_Output &) = delete;
      Raven_Disconnect_Output & operator=(const Raven_Disconnect_Output &) = delete;

    public:
      ZENI_RETE_LINKAGE Raven_Disconnect_Output(const std::shared_ptr<Concurrency::Maester> &recipient, const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output)
        : Raven(recipient), m_network(network), m_output(output)
      {
      }

      ZENI_RETE_LINKAGE std::shared_ptr<Network> get_Network() const {
        return m_network;
      }

      ZENI_RETE_LINKAGE std::shared_ptr<Node> get_output() const {
        return m_output;
      }

    private:
      const std::shared_ptr<Network> m_network;
      const std::shared_ptr<Node> m_output;
    };

  }

}

#endif
