#include "Zeni/Concurrency/Internal/Job_Queue_Impl.hpp"

#include "Zeni/Concurrency/IJob.hpp"
#include "Zeni/Concurrency/Worker_Threads.hpp"

#if ZENI_CONCURRENCY != ZENI_CONCURRENCY_NONE
#include <thread>
#endif

namespace Zeni::Concurrency {

  Job_Queue_Impl::Job_Queue_Impl(Worker_Threads * const worker_threads) noexcept
    : m_worker_threads(worker_threads)
  {
  }

  std::shared_ptr<Job_Queue_Impl> Job_Queue_Impl::Create(Worker_Threads * const worker_threads) noexcept {
    class Friendly_Job_Queue_Impl : public Job_Queue_Impl {
    public:
      Friendly_Job_Queue_Impl(Worker_Threads * const worker_threads) : Job_Queue_Impl(worker_threads) {}
    };

    return std::shared_ptr<Friendly_Job_Queue_Impl>(new Friendly_Job_Queue_Impl(worker_threads));
  }

  std::shared_ptr<IJob> Job_Queue_Impl::try_take_one(const bool is_already_awake) noexcept {
    std::shared_ptr<IJob> job;
    if (!m_jobs.try_pop(job))
      return nullptr;

#if ZENI_CONCURRENCY != ZENI_CONCURRENCY_NONE
    if (!is_already_awake)
      m_worker_threads->worker_awakened();

    m_worker_threads->job_removed();
#endif

    job->set_Job_Queue(shared_from_this());

    return job;
  }

  void Job_Queue_Impl::give_one(const std::shared_ptr<IJob> job) noexcept(false) {
#if ZENI_CONCURRENCY != ZENI_CONCURRENCY_NONE
    m_worker_threads->jobs_inserted(1);
#endif

    m_jobs.push(job);
  }

  void Job_Queue_Impl::give_many(std::vector<std::shared_ptr<IJob>> &&jobs) noexcept(false) {
#if ZENI_CONCURRENCY != ZENI_CONCURRENCY_NONE
    m_worker_threads->jobs_inserted(int64_t(jobs.size()));
#endif

    for (auto job : jobs)
      m_jobs.push(job);
  }

  void Job_Queue_Impl::give_many(const std::vector<std::shared_ptr<IJob>> &jobs) noexcept(false) {
#if ZENI_CONCURRENCY != ZENI_CONCURRENCY_NONE
    m_worker_threads->jobs_inserted(int64_t(jobs.size()));
#endif

    for (auto job : jobs)
      m_jobs.push(job);
  }

}
