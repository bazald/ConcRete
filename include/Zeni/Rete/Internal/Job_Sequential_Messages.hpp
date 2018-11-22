#ifndef ZENI_RETE_JOB_SEQUENTIAL_MESSAGES_HPP
#define ZENI_RETE_JOB_SEQUENTIAL_MESSAGES_HPP

#include "Message.hpp"
#include "../Node.hpp"

namespace Zeni::Rete {

  class Job_Sequential_Messages : public Concurrency::Job {
    Job_Sequential_Messages(const Job_Sequential_Messages &) = delete;
    Job_Sequential_Messages & operator=(const Job_Sequential_Messages &) = delete;

  public:
    ZENI_RETE_LINKAGE Job_Sequential_Messages(const std::shared_ptr<Network> network, const std::pair<const std::shared_ptr<Message>, const std::shared_ptr<Message>> messages);

    void execute() noexcept override;

    const std::pair<const std::shared_ptr<Message>, const std::shared_ptr<Message>> messages;
  };

}

#endif
