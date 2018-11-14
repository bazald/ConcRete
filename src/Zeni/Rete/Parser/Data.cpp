#include "Zeni/Rete/Parser/Data.hpp"

#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete::PEG {

  Data::Production::Production(const std::shared_ptr<Network> network_, const std::shared_ptr<Concurrency::Job_Queue> job_queue_, const bool user_command_)
    : network(network_), job_queue(job_queue_), user_command(user_command_)
  {
    lhs.push(decltype(lhs)::value_type());
  }

  Data::Production::~Production() {
    while (!lhs.empty()) {
      while (!lhs.top().empty()) {
        lhs.top().top().first.first->send_disconnect_from_parents(network, job_queue);
        lhs.top().pop();
      }
      lhs.pop();
    }
  }

  Data::Data(const std::shared_ptr<Network> network_, const std::shared_ptr<Concurrency::Job_Queue> job_queue_, const bool user_command_)
    : network(network_), job_queue(job_queue_)
  {
    productions.push(std::make_shared<Production>(network_, job_queue_, user_command_));
  }

}
