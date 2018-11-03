#include "Zeni/Rete/Internal/Message_Sequential_Pair.hpp"

namespace Zeni::Rete {

  Message_Sequential_Pair::Message_Sequential_Pair(const std::shared_ptr<Network> network, const std::pair<const std::shared_ptr<Message>, const std::shared_ptr<Message>> messages_)
  : Message(std::dynamic_pointer_cast<Node>(messages_.first->get_recipient()), network),
    messages(messages_)
  {
  }

  void Message_Sequential_Pair::receive() const {
    messages.first->set_Job_Queue(get_Job_Queue());
    messages.second->set_Job_Queue(get_Job_Queue());
    messages.first->receive();
    messages.second->receive();
  }

}
