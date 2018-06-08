#ifndef ZENI_RETE_MESSAGE_HPP
#define ZENI_RETE_MESSAGE_HPP

#include "Zeni/Concurrency/Message.hpp"
#include "Internal/Linkage.hpp"

namespace Zeni::Rete {

  class Network;
  class Node;

  class Message : public Concurrency::Message {
    Message(const Message &) = delete;
    Message & operator=(const Message &) = delete;

  public:
    ZENI_RETE_LINKAGE Message(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network);

    ZENI_RETE_LINKAGE virtual void receive() const = 0;

    const std::shared_ptr<Network> network;
  };

}

#endif
