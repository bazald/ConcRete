#ifndef ZENI_CONCURRENCY_JOB_HPP
#define ZENI_CONCURRENCY_JOB_HPP

#include "IJob.hpp"

#include "Job_Queue.hpp"

namespace Zeni::Concurrency {
  class Job;
}

namespace std {
  template class ZENI_CONCURRENCY_LINKAGE std::weak_ptr<Zeni::Concurrency::Job>;
}

namespace Zeni::Concurrency {

  /// A Job virtual base class
  class ZENI_CONCURRENCY_LINKAGE Job : public IJob, public std::enable_shared_from_this<Job> {
    Job(const Job &) = delete;
    Job & operator=(const Job &) = delete;

  public:
    Job() = default;

    const std::shared_ptr<Job_Queue> & get_Job_Queue() const noexcept override;

  private:
    void set_Job_Queue(const std::shared_ptr<Job_Queue> &job_queue) noexcept override;
    void set_Job_Queue(std::shared_ptr<Job_Queue> &&job_queue) noexcept override;

    std::shared_ptr<Job_Queue> m_job_queue;
  };

}

#endif
