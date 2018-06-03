#ifndef ZENI_CONCURRENCY_JOB_H
#define ZENI_CONCURRENCY_JOB_H

#include "Linkage.hpp"

#include <memory>

namespace Zeni::Concurrency {

  class Job_Pimpl;
  class Job_Queue;
  class Job_Queue_Pimpl;

  /// A Job virtual base class
  class Job : public std::enable_shared_from_this<Job> {
    Job(const Job &) = delete;
    Job & operator=(const Job &) = delete;

    static const int m_pimpl_size = 16;
    static const int m_pimpl_align = 8;
    const Job_Pimpl * get_pimpl() const noexcept;
    Job_Pimpl * get_pimpl() noexcept;

  public:
    ZENI_CONCURRENCY_LINKAGE Job() noexcept;
    ZENI_CONCURRENCY_LINKAGE ~Job() noexcept;

    ZENI_CONCURRENCY_LINKAGE const std::shared_ptr<Job_Queue> & get_Job_Queue() const noexcept;

    /// The function that gets called by whichever worker pulls this Job off of the Job_Queue
    ZENI_CONCURRENCY_LINKAGE virtual void execute() noexcept = 0;

  private:
    friend Job_Queue_Pimpl;
    void set_Job_Queue(const std::shared_ptr<Job_Queue> job_queue) noexcept;

    alignas(m_pimpl_align) char m_pimpl_storage[m_pimpl_size];
  };

}

#endif
