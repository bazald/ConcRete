#include "Zeni/Rete/Internal/Message.hpp"

#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  Message::Message(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network_)
    : Concurrency::Message(recipient), network(network_)
  {
  }

}
