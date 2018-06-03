#include "Zeni/Concurrency/Job.hpp"

#include "Zeni/Utility.hpp"

namespace Zeni::Concurrency {

  class Job_Pimpl {
    Job_Pimpl(const Job_Pimpl &) = delete;
    Job_Pimpl & operator=(const Job_Pimpl &) = delete;

  public:
    Job_Pimpl() noexcept
    {
    }

    const std::shared_ptr<Job_Queue> & get_Job_Queue() const noexcept {
      return m_job_queue;
    }

    void set_Job_Queue(const std::shared_ptr<Job_Queue> job_queue) noexcept {
      m_job_queue = job_queue;
    }

  private:
    std::shared_ptr<Job_Queue> m_job_queue;
  };

  const Job_Pimpl * Job::get_pimpl() const noexcept {
    return reinterpret_cast<const Job_Pimpl *>(m_pimpl_storage);
  }

  Job_Pimpl * Job::get_pimpl() noexcept {
    return reinterpret_cast<Job_Pimpl *>(m_pimpl_storage);
  }

  Job::Job() noexcept {
    new (&m_pimpl_storage) Job_Pimpl;
  }

  Job::~Job() noexcept {
    static_assert(std::alignment_of<Job_Pimpl>::value <= Job::m_pimpl_align, "Job::m_pimpl_align is too low.");
    ZENI_STATIC_WARNING(std::alignment_of<Job_Pimpl>::value >= Job::m_pimpl_align, "Job::m_pimpl_align is too high.");

    static_assert(sizeof(Job_Pimpl) <= sizeof(Job::m_pimpl_storage), "Job::m_pimpl_size too low.");
    ZENI_STATIC_WARNING(sizeof(Job_Pimpl) >= sizeof(Job::m_pimpl_storage), "Job::m_pimpl_size too high.");

    get_pimpl()->~Job_Pimpl();
  }

  const std::shared_ptr<Job_Queue> & Job::get_Job_Queue() const noexcept {
    return get_pimpl()->get_Job_Queue();
  }

  void Job::set_Job_Queue(const std::shared_ptr<Job_Queue> job_queue) noexcept {
    get_pimpl()->set_Job_Queue(job_queue);
  }

}
