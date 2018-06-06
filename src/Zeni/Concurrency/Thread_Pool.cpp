#include "Zeni/Concurrency/Thread_Pool.hpp"

#include "Zeni/Concurrency/IJob.hpp"
#include "Zeni/Concurrency/IJob_Queue.hpp"

#include <algorithm>
#include <cassert>
#include <list>
#include <system_error>

namespace Zeni::Concurrency {

#ifndef DISABLE_MULTITHREADING
  void worker(Thread_Pool * const thread_pool) noexcept;
#endif

  std::shared_ptr<Thread_Pool> Thread_Pool::Create() noexcept(false) {
    class Friendly_Thread_Pool : public Thread_Pool {
    public:
      Friendly_Thread_Pool() : Thread_Pool() {}
    };

    return std::make_shared<Friendly_Thread_Pool>();
  }

  std::shared_ptr<Thread_Pool> Thread_Pool::Create(const int16_t num_threads) noexcept(false) {
    class Friendly_Thread_Pool : public Thread_Pool {
    public:
      Friendly_Thread_Pool(const int16_t num_threads) : Thread_Pool() {}
    };

    return std::make_shared<Friendly_Thread_Pool>(num_threads);
  }

#ifdef DISABLE_MULTITHREADING
  Thread_Pool::Thread_Pool(Thread_Pool * const pub_this) noexcept(false)
    : m_job_queue(Job_Queue::Create(pub_this))
  {
  }

  Thread_Pool::Thread_Pool(Thread_Pool * const pub_this, const int16_t) noexcept(false)
    : m_job_queue(Job_Queue::Create(pub_this))
  {
  }
#else
  Thread_Pool::Thread_Pool() noexcept(false)
    : Thread_Pool(std::thread::hardware_concurrency())
  {
  }

  Thread_Pool::Thread_Pool(const int16_t num_threads) noexcept(false)
    : m_job_queue(IJob_Queue::Create(this))
  {
    m_worker_threads.reserve(num_threads);
    m_job_queues.reserve(num_threads);
    m_job_queues.emplace_back(std::make_pair(std::this_thread::get_id(), m_job_queue));
    for (int16_t num_threads_created = 1; num_threads_created < num_threads; ++num_threads_created) {
      std::shared_ptr<std::thread> new_thread;

      try {
        auto new_job_queue = IJob_Queue::Create(this);
        new_thread = std::make_shared<std::thread>(worker, this);
        m_worker_threads.emplace_back(new_thread);
        m_job_queues.emplace_back(std::make_pair(new_thread->get_id(), new_job_queue));
      }
      catch (const std::system_error &) {
        if (new_thread) {
          m_failed_thread_id.store(new_thread->get_id(), std::memory_order_release);
          if (!m_worker_threads.empty() && *m_worker_threads.crbegin() == new_thread)
            m_worker_threads.pop_back();
          new_thread->join();
        }
        break;
      }
      catch (...) {
        if (new_thread) {
          m_failed_thread_id.store(new_thread->get_id(), std::memory_order_release);
          if (!m_worker_threads.empty() && *m_worker_threads.crbegin() == new_thread)
            m_worker_threads.pop_back();
          new_thread->join();
        }
        throw;
      }
    }
    m_initialized.store(true, std::memory_order_release);
  }
#endif

  Thread_Pool::~Thread_Pool() noexcept {
    finish_jobs();

#ifndef DISABLE_MULTITHREADING
    m_nonempty_job_queues.fetch_sub(1, std::memory_order_relaxed); // If everyone behaves, guaranteed to go negative to initiate termination in worker threads
    for (auto &worker : m_worker_threads)
      worker->join();
#endif
  }

  std::shared_ptr<IJob_Queue> Thread_Pool::get_main_Job_Queue() const noexcept {
    return m_job_queue;
  }

#ifdef DISABLE_MULTITHREADING
  void Thread_Pool::finish_jobs() noexcept(false) {
    while (std::shared_ptr<IJob> job = m_job_queue->try_take_one(true))
      job->execute();
  }
#else
  void Thread_Pool::finish_jobs() noexcept(false) {
    for (;;) {
      int16_t nonempty_job_queues = m_nonempty_job_queues.load(std::memory_order_acquire);
      if (nonempty_job_queues <= 0) {
        // Termination condition OR simply out of jobs
        int16_t awake_workers = m_awake_workers.load(std::memory_order_acquire);
        if (awake_workers == 0)
          break;
        else {
          std::this_thread::yield();
          continue;
        }
      }

      for (;;) {
        auto jqt = m_job_queues.begin();
        while (const std::shared_ptr<IJob> job = (*jqt).second->try_take_one(true))
          job->execute();

        while (jqt != m_job_queues.end()) {
          if (const std::shared_ptr<IJob> job = (*jqt).second->try_take_one(true)) {
            job->execute();
            break;
          }
          else
            ++jqt;
        }

        if (jqt == m_job_queues.end())
          break;
      }
    }
  }

  void Thread_Pool::worker_awakened() noexcept {
    m_awake_workers.fetch_add(1, std::memory_order_relaxed);
  }

  void Thread_Pool::job_queue_emptied() noexcept {
    m_nonempty_job_queues.fetch_sub(1, std::memory_order_relaxed);
  }

  void Thread_Pool::job_queue_nonemptied() noexcept {
    m_nonempty_job_queues.fetch_add(1, std::memory_order_relaxed);
  }

  void Thread_Pool::worker_thread_work() noexcept {
    const auto thread_id = std::this_thread::get_id();
    while (!m_initialized.load(std::memory_order_acquire)) {
      if (m_failed_thread_id.load(std::memory_order_acquire) == thread_id)
        return;
    }

    std::vector<std::shared_ptr<IJob_Queue>> job_queues;
    {
      auto mine = std::find_if(m_job_queues.cbegin(), m_job_queues.cend(), [](const auto &value) {return value.first == std::this_thread::get_id();});
      assert(mine != m_job_queues.cend());

      // Initialize other_job_queues to the queues after mine, followed by the ones before
      job_queues.reserve(m_job_queues.size() - 1);
      for (auto jqt = mine; jqt != m_job_queues.cend(); ++jqt)
        job_queues.emplace_back(jqt->second);
      for (auto jqt = m_job_queues.cbegin(); jqt != mine; ++jqt)
        job_queues.emplace_back(jqt->second);
    }

    bool is_awake = false;
    for (;;) {
      int16_t nonempty_job_queues = m_nonempty_job_queues.load(std::memory_order_acquire);
      if (nonempty_job_queues < 0)
        break; // Termination condition
      if (nonempty_job_queues == 0) {
        // Must be a worker thread, so safe to continue
        if (is_awake) {
          is_awake = false;
          m_awake_workers.fetch_sub(1, std::memory_order_relaxed);
          std::this_thread::yield();
        }
        continue;
      }

      for(;;) {
        auto jqt = job_queues.begin();
        while (const std::shared_ptr<IJob> job = (*jqt)->try_take_one(is_awake)) {
          is_awake = true;
          job->execute();
        }

        while(jqt != job_queues.end()) {
          if (const std::shared_ptr<IJob> job = (*jqt)->try_take_one(is_awake)) {
            is_awake = true;
            job->execute();
            break;
          }
          else
            ++jqt;
        }

        if(jqt == job_queues.end())
          break;
      }
    }

    if (is_awake) {
      is_awake = false;
      m_awake_workers.fetch_sub(1, std::memory_order_relaxed);
    }
  }
#endif

#ifdef DISABLE_MULTITHREADING
  static int64_t g_total_thread_count = 0;
#else
  static std::atomic_int64_t g_total_thread_count = 0;

  void worker(Thread_Pool * const thread_pool) noexcept {
    const class Count {
    public:
      Count() {
        g_total_thread_count.fetch_add(1, std::memory_order_relaxed);
      }

      ~Count() {
        g_total_thread_count.fetch_sub(1, std::memory_order_relaxed);
      }
    } count;

    std::shared_ptr<Job_Queue> my_job_queue;
    std::vector<std::shared_ptr<Job_Queue>> other_job_queues;

    thread_pool->worker_thread_work();
  }
#endif

  int64_t Thread_Pool::get_total_workers() noexcept {
#ifdef DISABLE_MULTITHREADING
    return 0;
#else
    return g_total_thread_count.load(std::memory_order_acquire);
#endif
  }

}
