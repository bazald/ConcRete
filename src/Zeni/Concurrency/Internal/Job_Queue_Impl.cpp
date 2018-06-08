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
#ifndef DISABLE_MULTITHREADING
    if (!m_has_jobs.load(std::memory_order_acquire))
      return nullptr;

    std::lock_guard<std::mutex> mutex_lock(m_mutex);
#endif

    if (m_jobs.empty())
      return nullptr;

    const std::shared_ptr<IJob> job = m_jobs.front().back();
    m_jobs.front().pop_back();

    if (m_jobs.front().empty())
      m_jobs.pop();

#ifndef DISABLE_MULTITHREADING
    if (!is_already_awake)
      m_worker_threads->worker_awakened();

    if (m_jobs.empty())
      m_worker_threads->job_queue_emptied();
#endif

    job->set_Job_Queue(shared_from_this());

    return job;
  }

  void Job_Queue_Impl::give_one(const std::shared_ptr<IJob> job) noexcept(false) {
#ifndef DISABLE_MULTITHREADING
    std::lock_guard<std::mutex> mutex_lock(m_mutex);
#endif

    [[maybe_unused]] const bool nonemptied = m_jobs.empty();

    m_jobs.emplace(1, job);

#ifndef DISABLE_MULTITHREADING
    if (nonemptied) {
      m_has_jobs.store(true, std::memory_order_release);
      m_worker_threads->job_queue_nonemptied();
    }
#endif
  }

  void Job_Queue_Impl::give_many(std::vector<std::shared_ptr<IJob>> &&jobs) noexcept(false) {
    if (jobs.empty())
      return;

#ifndef DISABLE_MULTITHREADING
    std::lock_guard<std::mutex> mutex_lock(m_mutex);
#endif

    [[maybe_unused]] const bool nonemptied = m_jobs.empty();

    m_jobs.emplace(std::move(jobs));

#ifndef DISABLE_MULTITHREADING
    if (nonemptied) {
      m_has_jobs.store(true, std::memory_order_release);
      m_worker_threads->job_queue_nonemptied();
    }
#endif
  }

  void Job_Queue_Impl::give_many(const std::vector<std::shared_ptr<IJob>> &jobs) noexcept(false) {
    if (jobs.empty())
      return;

#ifndef DISABLE_MULTITHREADING
    std::lock_guard<std::mutex> mutex_lock(m_mutex);
#endif

    [[maybe_unused]] const bool nonemptied = m_jobs.empty();

    m_jobs.emplace(jobs);

#ifndef DISABLE_MULTITHREADING
    if (nonemptied) {
      m_has_jobs.store(true, std::memory_order_release);
      m_worker_threads->job_queue_nonemptied();
    }
#endif
  }

}
