#include "Zeni/Rete/Raven.hpp"

#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  Raven::Raven(const std::shared_ptr<Pseudonode> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> sender)
    : Concurrency::Raven(recipient), m_network(network), m_sender(sender)
  {
  }

  const std::shared_ptr<Network> & Raven::get_Network() const {
    return m_network;
  }

  const std::shared_ptr<const Node> & Raven::get_sender() const {
    return m_sender;
  }

}
