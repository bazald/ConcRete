#include "Zeni/Rete/Message.hpp"

#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  Message::Message(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> sender)
    : Concurrency::Message(recipient), m_network(network), m_sender(sender)
  {
  }

  const std::shared_ptr<Network> & Message::get_Network() const {
    return m_network;
  }

  const std::shared_ptr<const Node> & Message::get_sender() const {
    return m_sender;
  }

}
