#ifndef ZENI_RETE_RAVEN_H
#define ZENI_RETE_RAVEN_H

#include "Zeni/Concurrency/Raven.hpp"
#include "Linkage.hpp"

namespace Zeni {

  namespace Rete {

    class Network;
    class Node;
    class Pseudonode;

    class Raven : public Concurrency::Raven {
      Raven(const Raven &) = delete;
      Raven & operator=(const Raven &) = delete;

    public:
      ZENI_RETE_LINKAGE Raven(const std::shared_ptr<Pseudonode> &recipient, const std::shared_ptr<Network> &network, const std::shared_ptr<const Node> &sender);

      ZENI_RETE_LINKAGE const std::shared_ptr<Network> & get_Network() const;

      ZENI_RETE_LINKAGE const std::shared_ptr<const Node> & get_sender() const;

      virtual void receive() const = 0;

    private:
      const std::shared_ptr<Network> m_network;
      const std::shared_ptr<const Node> m_sender;
    };

  }

}

#endif
