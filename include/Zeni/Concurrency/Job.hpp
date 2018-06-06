#ifndef ZENI_CONCURRENCY_JOB_HPP
#define ZENI_CONCURRENCY_JOB_HPP

#include "IJob.hpp"

namespace Zeni::Concurrency {

  class Job_Queue_Pimpl;

  /// A Job virtual base class
  class Job : public IJob, public std::enable_shared_from_this<Job> {
    Job(const Job &) = delete;
    Job & operator=(const Job &) = delete;

  public:
    ZENI_CONCURRENCY_LINKAGE Job() = default;

    ZENI_CONCURRENCY_LINKAGE const std::shared_ptr<Job_Queue> & get_Job_Queue() const noexcept override;

  private:
    ZENI_CONCURRENCY_LINKAGE void set_Job_Queue(const std::shared_ptr<Job_Queue> &job_queue) noexcept override;
    ZENI_CONCURRENCY_LINKAGE void set_Job_Queue(std::shared_ptr<Job_Queue> &&job_queue) noexcept override;

    std::shared_ptr<Job_Queue> m_job_queue;
  };

}

#endif
