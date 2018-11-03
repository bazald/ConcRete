#ifndef ZENI_RETE_MESSAGE_SEQUENTIAL_PAIR_HPP
#define ZENI_RETE_MESSAGE_SEQUENTIAL_PAIR_HPP

#include "Message.hpp"
#include "../Node.hpp"

namespace Zeni::Rete {

  class Message_Sequential_Pair : public Message {
    Message_Sequential_Pair(const Message_Sequential_Pair &) = delete;
    Message_Sequential_Pair & operator=(const Message_Sequential_Pair &) = delete;

  public:
    ZENI_RETE_LINKAGE Message_Sequential_Pair(const std::shared_ptr<Network> network, const std::pair<const std::shared_ptr<Message>, const std::shared_ptr<Message>> messages);

    ZENI_RETE_LINKAGE const std::shared_ptr<const Node> & get_sender() const;

    void receive() const override;

    const std::pair<const std::shared_ptr<Message>, const std::shared_ptr<Message>> messages;
  };

}

#endif
