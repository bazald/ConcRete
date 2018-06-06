#include "Zeni/Concurrency/Job.hpp"

namespace Zeni::Concurrency {

  const std::shared_ptr<Job_Queue> & Job::get_Job_Queue() const noexcept {
    return m_job_queue;
  }

  void Job::set_Job_Queue(const std::shared_ptr<Job_Queue> &job_queue) noexcept {
    m_job_queue = job_queue;
  }

  void Job::set_Job_Queue(std::shared_ptr<Job_Queue> &&job_queue) noexcept {
    m_job_queue = std::move(job_queue);
  }

}
