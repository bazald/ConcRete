#ifndef ZENI_RETE_JOB_SEQUENTIAL_MESSAGES_HPP
#define ZENI_RETE_JOB_SEQUENTIAL_MESSAGES_HPP

#include "Message.hpp"
#include "../Node.hpp"

namespace Zeni::Rete {

  class Job_Sequential_Messages : public Concurrency::Job {
    Job_Sequential_Messages(const Job_Sequential_Messages &) = delete;
    Job_Sequential_Messages & operator=(const Job_Sequential_Messages &) = delete;

  public:
    ZENI_RETE_LINKAGE Job_Sequential_Messages(const std::shared_ptr<Network> network, const std::shared_ptr<Message> first, const std::shared_ptr<Message> second);

    void execute() noexcept override;

    const std::shared_ptr<Message> first;
    const std::shared_ptr<Message> second;
  };

  /// Will pass the second message only on the condition that the counter decrements to zero after the first message
  class Job_Sequential_Messages_Countdown : public Job_Sequential_Messages {
    Job_Sequential_Messages_Countdown(const Job_Sequential_Messages_Countdown &) = delete;
    Job_Sequential_Messages_Countdown & operator=(const Job_Sequential_Messages_Countdown &) = delete;

  public:
    ZENI_RETE_LINKAGE Job_Sequential_Messages_Countdown(const std::shared_ptr<Network> network, const std::shared_ptr<Message> first, const std::shared_ptr<Message> second, const std::shared_ptr<std::atomic_int64_t> counter);

    void execute() noexcept override;

  private:
    const std::shared_ptr<std::atomic_int64_t> counter;
  };

}

#endif
