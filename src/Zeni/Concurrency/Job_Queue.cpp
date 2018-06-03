#include "Zeni/Concurrency/Job_Queue.hpp"

#include "Zeni/Concurrency/Job.hpp"
#include "Zeni/Concurrency/Thread_Pool.hpp"
#include "Zeni/Utility.hpp"

#ifndef DISABLE_MULTITHREADING
#include <atomic>
#include <mutex>
#include <thread>
#endif

#include <queue>

namespace Zeni::Concurrency {

  class Job_Queue_Pimpl {
  public:
    Job_Queue_Pimpl(Thread_Pool * const thread_pool) noexcept
      : m_thread_pool(thread_pool)
    {
    }

    std::shared_ptr<Job> try_take_one(const std::shared_ptr<Job_Queue> pub_this, const bool is_already_awake) noexcept {
#ifndef DISABLE_MULTITHREADING
      if (!m_has_jobs.load(std::memory_order_acquire))
        return nullptr;

      std::unique_lock<std::mutex> mutex_lock(m_mutex);
#endif

      if (m_jobs.empty())
        return nullptr;

      const std::shared_ptr<Job> job = m_jobs.front().back();
      m_jobs.front().pop_back();

      if (m_jobs.front().empty())
        m_jobs.pop();

      if (!is_already_awake)
        m_thread_pool->worker_awakened();

      if (m_jobs.empty())
        m_thread_pool->job_queue_emptied();

      job->set_Job_Queue(pub_this);

      return job;
    }

    void give_one(const std::shared_ptr<Job> job) noexcept(false) {
#ifndef DISABLE_MULTITHREADING
      std::unique_lock<std::mutex> mutex_lock(m_mutex);
#endif

      [[maybe_unused]] const bool nonemptied = m_jobs.empty();

      m_jobs.emplace(1, job);

#ifndef DISABLE_MULTITHREADING
      if (nonemptied) {
        m_has_jobs.store(true, std::memory_order_release);
        m_thread_pool->job_queue_nonemptied();
      }
#endif
    }

    void give_many(std::vector<std::shared_ptr<Job>> &&jobs) noexcept(false) {
      if (jobs.empty())
        return;

#ifndef DISABLE_MULTITHREADING
      std::unique_lock<std::mutex> mutex_lock(m_mutex);
#endif

      [[maybe_unused]] const bool nonemptied = m_jobs.empty();

      m_jobs.emplace(std::move(jobs));

#ifndef DISABLE_MULTITHREADING
      if (nonemptied) {
        m_has_jobs.store(true, std::memory_order_release);
        m_thread_pool->job_queue_nonemptied();
      }
#endif
    }

    void give_many(const std::vector<std::shared_ptr<Job>> &jobs) noexcept(false) {
      if (jobs.empty())
        return;

#ifndef DISABLE_MULTITHREADING
      std::unique_lock<std::mutex> mutex_lock(m_mutex);
#endif

      [[maybe_unused]] const bool nonemptied = m_jobs.empty();

      m_jobs.emplace(jobs);

#ifndef DISABLE_MULTITHREADING
      if (nonemptied) {
        m_has_jobs.store(true, std::memory_order_release);
        m_thread_pool->job_queue_nonemptied();
      }
#endif
    }

  private:
#ifndef DISABLE_MULTITHREADING
    std::atomic_bool m_has_jobs = false;
    std::mutex m_mutex;
#endif
    Thread_Pool * const m_thread_pool;
    std::queue<std::vector<std::shared_ptr<Job>>> m_jobs;
  };

  const Job_Queue_Pimpl * Job_Queue::get_pimpl() const noexcept {
    return reinterpret_cast<const Job_Queue_Pimpl *>(m_pimpl_storage);
  }

  Job_Queue_Pimpl * Job_Queue::get_pimpl() noexcept {
    return reinterpret_cast<Job_Queue_Pimpl *>(m_pimpl_storage);
  }

  Job_Queue::Job_Queue(Thread_Pool * const thread_pool) noexcept {
    new (&m_pimpl_storage) Job_Queue_Pimpl(thread_pool);
  }

  Job_Queue::~Job_Queue() noexcept {
    static_assert(std::alignment_of<Job_Queue_Pimpl>::value <= Job_Queue::m_pimpl_align, "Job_Queue::m_pimpl_align is too low.");
    ZENI_STATIC_WARNING(std::alignment_of<Job_Queue_Pimpl>::value >= Job_Queue::m_pimpl_align, "Job_Queue::m_pimpl_align is too high.");

    static_assert(sizeof(Job_Queue_Pimpl) <= sizeof(Job_Queue::m_pimpl_storage), "Job_Queue::m_pimpl_size too low.");
    ZENI_STATIC_WARNING(sizeof(Job_Queue_Pimpl) >= sizeof(Job_Queue::m_pimpl_storage), "Job_Queue::m_pimpl_size too high.");
    
    get_pimpl()->~Job_Queue_Pimpl();
  }

  std::shared_ptr<Job_Queue> Job_Queue::Create(Thread_Pool * const thread_pool) noexcept {
    class Friendly_Job_Queue : public Job_Queue {
    public:
      Friendly_Job_Queue(Thread_Pool * const thread_pool) : Job_Queue(thread_pool) {}
    };

    return std::make_shared<Friendly_Job_Queue>(thread_pool);
  }

  std::shared_ptr<Job> Job_Queue::try_take_one(const bool is_already_awake) noexcept {
    return get_pimpl()->try_take_one(shared_from_this(), is_already_awake);
  }

  void Job_Queue::give_one(const std::shared_ptr<Job> job) noexcept(false) {
    return get_pimpl()->give_one(job);
  }

  void Job_Queue::give_many(std::vector<std::shared_ptr<Job>> &&jobs) noexcept(false) {
    return get_pimpl()->give_many(std::move(jobs));
  }

  void Job_Queue::give_many(const std::vector<std::shared_ptr<Job>> &jobs) noexcept(false) {
    return get_pimpl()->give_many(jobs);
  }

}
