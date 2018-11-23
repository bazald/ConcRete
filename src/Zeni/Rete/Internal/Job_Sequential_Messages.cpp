#include "Zeni/Rete/Internal/Job_Sequential_Messages.hpp"

namespace Zeni::Rete {

  Job_Sequential_Messages::Job_Sequential_Messages(const std::shared_ptr<Network> network, const std::pair<const std::shared_ptr<Message>, const std::shared_ptr<Message>> messages_)
    : messages(messages_)
  {
  }

  void Job_Sequential_Messages::execute() noexcept {
    messages.first->set_Job_Queue(get_Job_Queue());
    messages.first->receive();

    messages.second->set_Job_Queue(get_Job_Queue());
    messages.second->receive();
  }

  Job_Sequential_Messages_Countdown::Job_Sequential_Messages_Countdown(const std::shared_ptr<Network> network, const std::pair<const std::shared_ptr<Message>, const std::shared_ptr<Message>> messages_, const std::shared_ptr<std::atomic_int64_t> counter_)
    : messages(messages_), counter(counter_)
  {
  }

  void Job_Sequential_Messages_Countdown::execute() noexcept {
    messages.first->set_Job_Queue(get_Job_Queue());
    messages.first->receive();

    if (counter->fetch_sub(1, std::memory_order_relaxed) == 1) {
      messages.second->set_Job_Queue(get_Job_Queue());
      messages.second->receive();
    }
  }

}
