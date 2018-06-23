#include "Zeni/Concurrency/Internal/Job_Queue_Impl.hpp"

#include "Zeni/Concurrency/IJob.hpp"
#include "Zeni/Concurrency/Worker_Threads.hpp"

#ifndef DISABLE_MULTITHREADING
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

    return std::make_shared<Friendly_Job_Queue_Impl>(worker_threads);
  }

  std::shared_ptr<IJob> Job_Queue_Impl::try_take_one(const bool is_already_awake) noexcept {
    std::shared_ptr<IJob> job;
    if (!m_jobs.try_pop(job))
      return nullptr;

#ifndef DISABLE_MULTITHREADING
    if (!is_already_awake)
      m_worker_threads->worker_awakened();

    m_worker_threads->job_removed();
#endif

    job->set_Job_Queue(shared_from_this());

    return job;
  }

  bool Job_Queue_Impl::try_reclaim() noexcept {
#ifndef DISABLE_MULTITHREADING
    if (!m_reclaim.load(std::memory_order_acquire))
      return false;
    m_reclaim.store(false, std::memory_order_release);
#endif
    return true;
  }

  void Job_Queue_Impl::give_one(const std::shared_ptr<IJob> job) noexcept(false) {
#ifndef DISABLE_MULTITHREADING
    m_worker_threads->jobs_inserted(1);
#endif

    m_jobs.push(job);
  }

  void Job_Queue_Impl::give_many(std::vector<std::shared_ptr<IJob>> &&jobs) noexcept(false) {
#ifndef DISABLE_MULTITHREADING
    m_worker_threads->jobs_inserted(int64_t(jobs.size()));
#endif

    for (auto job : jobs)
      m_jobs.push(job);
  }

  void Job_Queue_Impl::give_many(const std::vector<std::shared_ptr<IJob>> &jobs) noexcept(false) {
#ifndef DISABLE_MULTITHREADING
    m_worker_threads->jobs_inserted(int64_t(jobs.size()));
#endif

    for (auto job : jobs)
      m_jobs.push(job);
  }

  void Job_Queue_Impl::set_reclaim() noexcept {
#ifndef DISABLE_MULTITHREADING
    m_reclaim.store(true, std::memory_order_release);
#endif
  }

}
