#include "Zeni/Rete/Internal/Job_Sequential_Messages.hpp"

namespace Zeni::Rete {

  Job_Sequential_Messages::Job_Sequential_Messages(const std::shared_ptr<Network> network, const std::shared_ptr<Message> first_, const std::shared_ptr<Message> second_)
    : first(first_), second(second_)
  {
  }

  void Job_Sequential_Messages::execute() noexcept {
    first->set_Job_Queue(get_Job_Queue());
    first->receive();

    second->set_Job_Queue(get_Job_Queue());
    second->receive();
  }

  Job_Sequential_Messages_Countdown::Job_Sequential_Messages_Countdown(const std::shared_ptr<Network> network, const std::shared_ptr<Message> first, const std::shared_ptr<Message> second, const std::shared_ptr<std::atomic_int64_t> counter_)
    : Job_Sequential_Messages(network, first, second), counter(counter_)
  {
  }

  void Job_Sequential_Messages_Countdown::execute() noexcept {
    first->set_Job_Queue(get_Job_Queue());
    first->receive();

    if (counter->fetch_sub(1, std::memory_order_relaxed) == 1) {
      second->set_Job_Queue(get_Job_Queue());
      second->receive();
    }
  }

}
