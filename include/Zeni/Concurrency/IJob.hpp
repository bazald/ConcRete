#ifndef ZENI_CONCURRENCY_IJOB_HPP
#define ZENI_CONCURRENCY_IJOB_HPP

#include "Linkage.hpp"

#include <memory>

namespace Zeni::Concurrency {

  class Job_Queue;

  /// A Job virtual base class
  class IJob {
    IJob(const IJob &) = delete;
    IJob & operator=(const IJob &) = delete;

  public:
    ZENI_CONCURRENCY_LINKAGE IJob() = default;

    ZENI_CONCURRENCY_LINKAGE virtual const std::shared_ptr<Job_Queue> & get_Job_Queue() const noexcept = 0;

    /// The function that gets called by whichever worker pulls this Job off of the Job_Queue
    ZENI_CONCURRENCY_LINKAGE virtual void execute() noexcept = 0;

  private:
    friend Job_Queue;
    virtual void set_Job_Queue(const std::shared_ptr<Job_Queue> &job_queue) noexcept = 0;
    virtual void set_Job_Queue(std::shared_ptr<Job_Queue> &&job_queue) noexcept = 0;
  };

}

#endif
