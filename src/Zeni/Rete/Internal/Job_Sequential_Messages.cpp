#include "Zeni/Rete/Internal/Job_Sequential_Messages.hpp"

namespace Zeni::Rete {

  Job_Sequential_Messages::Job_Sequential_Messages(const std::shared_ptr<Network> network, const std::pair<const std::shared_ptr<Message>, const std::shared_ptr<Message>> messages_)
  : messages(messages_)
  {
  }

  void Job_Sequential_Messages::execute() noexcept {
    messages.first->set_Job_Queue(get_Job_Queue());
    messages.second->set_Job_Queue(get_Job_Queue());
    messages.first->receive();
    messages.second->receive();
  }

}
