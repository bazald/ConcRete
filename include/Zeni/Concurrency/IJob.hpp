#ifndef ZENI_CONCURRENCY_IJOB_HPP
#define ZENI_CONCURRENCY_IJOB_HPP

#include "Internal/Linkage.hpp"

#include <memory>

namespace Zeni::Concurrency {

  class Job_Queue;
  class Job_Queue_Impl;

  /// A Job virtual base class
  class ZENI_CONCURRENCY_LINKAGE IJob {
    IJob(const IJob &) = delete;
    IJob & operator=(const IJob &) = delete;

  public:
    IJob() = default;

    virtual const std::shared_ptr<Job_Queue> & get_Job_Queue() const noexcept = 0;

    /// The function that gets called by whichever worker pulls this Job off of the Job_Queue
    virtual void execute() noexcept = 0;

  private:
    friend Job_Queue_Impl;
    virtual void set_Job_Queue(const std::shared_ptr<Job_Queue> &job_queue) noexcept = 0;
    virtual void set_Job_Queue(std::shared_ptr<Job_Queue> &&job_queue) noexcept = 0;
  };

}

#endif
