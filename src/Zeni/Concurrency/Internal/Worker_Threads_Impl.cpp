#include "Zeni/Concurrency/Internal/Worker_Threads_Impl.hpp"

#include "Zeni/Concurrency/IJob.hpp"
#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Concurrency/Memory_Pool.hpp"
#include "Zeni/Concurrency/Memory_Pools.hpp"
#include "Zeni/Concurrency/Reclamation_Stacks.hpp"

#include <algorithm>
#include <cassert>
#include <list>
#include <random>
#include <system_error>

namespace Zeni::Concurrency {

#ifndef DISABLE_MULTITHREADING
  void worker(Worker_Threads_Impl * const worker_threads) noexcept;
#endif

  std::shared_ptr<Worker_Threads_Impl> Worker_Threads_Impl::Create() noexcept(false) {
    class Friendly_Worker_Threads_Impl : public Worker_Threads_Impl
    {
    };

    return std::make_shared<Friendly_Worker_Threads_Impl>();
  }

  std::shared_ptr<Worker_Threads_Impl> Worker_Threads_Impl::Create(const int16_t num_threads) noexcept(false) {
    class Friendly_Worker_Threads_Impl : public Worker_Threads_Impl {
    public:
      Friendly_Worker_Threads_Impl(const int16_t num_threads) : Worker_Threads_Impl(num_threads) {}
    };

    return std::make_shared<Friendly_Worker_Threads_Impl>(num_threads);
  }

#ifdef DISABLE_MULTITHREADING
  Worker_Threads_Impl::Worker_Threads_Impl() noexcept(false)
    : m_job_queue(Job_Queue::Create(this))
  {
  }

  Worker_Threads_Impl::Worker_Threads_Impl(const int16_t) noexcept(false)
    : m_job_queue(Job_Queue::Create(this))
  {
  }
#else
  Worker_Threads_Impl::Worker_Threads_Impl() noexcept(false)
    : Worker_Threads_Impl(std::thread::hardware_concurrency())
  {
  }

  Worker_Threads_Impl::Worker_Threads_Impl(const int16_t num_threads) noexcept(false)
    : m_job_queue(Job_Queue::Create(this))
  {
    m_worker_threads.reserve(num_threads);
    m_job_queues.reserve(num_threads);
    m_job_queues.emplace_back(std::make_pair(std::this_thread::get_id(), m_job_queue));
    for (int16_t num_threads_created = 1; num_threads_created < num_threads; ++num_threads_created) {
      std::shared_ptr<std::thread> new_thread;

      try {
        auto new_job_queue = Job_Queue::Create(this);
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

  Worker_Threads_Impl::~Worker_Threads_Impl() noexcept {
    finish_jobs();

#ifndef DISABLE_MULTITHREADING
    m_nonempty_job_queues.fetch_sub(1, std::memory_order_relaxed); // If everyone behaves, guaranteed to go negative to initiate termination in worker threads
    for (auto &worker : m_worker_threads)
      worker->join();
#endif
  }

  std::shared_ptr<Job_Queue> Worker_Threads_Impl::get_main_Job_Queue() const noexcept {
    return m_job_queue;
  }

#ifdef DISABLE_MULTITHREADING
  void Worker_Threads_Impl::finish_jobs() noexcept(false) {
    while (std::shared_ptr<IJob> job = m_job_queue->try_take_one(true))
      job->execute();

    Reclamation_Stacks::get_stack()->reclaim();
    Memory_Pools::get_pool()->clear();
  }
#else
  void Worker_Threads_Impl::finish_jobs() noexcept(false) {
    for (;;) {
      if (m_nonempty_job_queues.load(std::memory_order_acquire) <= 0) {
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

    for (auto &job_queue : m_job_queues)
      job_queue.second->set_reclaim();
    m_reclaims_remaining.store(int16_t(m_worker_threads.size()));

    Reclamation_Stacks::get_stack()->reclaim();
    Memory_Pools::get_pool()->clear();

    while (m_reclaims_remaining.load(std::memory_order_acquire) != 0);
  }

  void Worker_Threads_Impl::worker_awakened() noexcept {
    m_awake_workers.fetch_add(1, std::memory_order_relaxed);
  }

  void Worker_Threads_Impl::job_queue_emptied() noexcept {
    m_nonempty_job_queues.fetch_sub(1, std::memory_order_relaxed);
  }

  void Worker_Threads_Impl::job_queue_nonemptied() noexcept {
    m_nonempty_job_queues.fetch_add(1, std::memory_order_relaxed);
  }

  void Worker_Threads_Impl::worker_thread_work() noexcept {
    const auto thread_id = std::this_thread::get_id();
    while (!m_initialized.load(std::memory_order_acquire)) {
      if (m_failed_thread_id.load(std::memory_order_acquire) == thread_id)
        return;
    }

    std::vector<std::shared_ptr<Job_Queue>> job_queues;
    std::shared_ptr<Job_Queue> my_job_queue;
    {
      auto mine = std::find_if(m_job_queues.cbegin(), m_job_queues.cend(), [](const auto &value) {return value.first == std::this_thread::get_id();});
      assert(mine != m_job_queues.cend());
      my_job_queue = mine->second;

      // Initialize job_queues to the queues mine and after, followed by the ones before
      job_queues.reserve(m_job_queues.size());
      for (auto jqt = mine; jqt != m_job_queues.cend(); ++jqt)
        job_queues.emplace_back(jqt->second);
      for (auto jqt = m_job_queues.cbegin(); jqt != mine; ++jqt)
        job_queues.emplace_back(jqt->second);

      std::shuffle(job_queues.begin(), job_queues.end(), std::mt19937(std::random_device()()));
    }

    bool is_awake = false;
    for (;;) {
      while (m_reclaims_remaining.load(std::memory_order_acquire) != 0) {
        if (my_job_queue->try_reclaim()) {
          Reclamation_Stacks::get_stack()->reclaim();
          Memory_Pools::get_pool()->clear();
          m_reclaims_remaining.fetch_sub(1, std::memory_order_relaxed);
        }
      }

      int16_t nonempty_job_queues = m_nonempty_job_queues.load(std::memory_order_relaxed);
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

  void worker(Worker_Threads_Impl * const worker_threads_impl) noexcept {
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

    worker_threads_impl->worker_thread_work();
  }
#endif

  int64_t Worker_Threads_Impl::get_total_workers() noexcept {
#ifdef DISABLE_MULTITHREADING
    return 0;
#else
    return g_total_thread_count.load(std::memory_order_acquire);
#endif
  }

}
