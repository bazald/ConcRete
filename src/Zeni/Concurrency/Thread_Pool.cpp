#include "Zeni/Concurrency/Thread_Pool.hpp"

#include "Zeni/Concurrency/Job.hpp"
#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Utility.hpp"

#include <algorithm>
#include <cassert>
#include <list>
#include <system_error>

#ifndef DISABLE_MULTITHREADING
#include <atomic>
#include <thread>
#endif

namespace Zeni::Concurrency {

#ifndef DISABLE_MULTITHREADING
  static void worker(Thread_Pool_Pimpl * const thread_pool) noexcept;
#endif

  class Thread_Pool_Pimpl {
    Thread_Pool_Pimpl(const Thread_Pool &) = delete;
    Thread_Pool_Pimpl & operator=(const Thread_Pool_Pimpl &) = delete;

  public:
#ifdef DISABLE_MULTITHREADING
    Thread_Pool_Pimpl(Thread_Pool * const pub_this) noexcept(false)
      : m_job_queue(Job_Queue::Create(pub_this))
    {
    }

    Thread_Pool_Pimpl(Thread_Pool * const pub_this, const int16_t) noexcept(false)
      : m_job_queue(Job_Queue::Create(pub_this))
    {
    }
#else
    Thread_Pool_Pimpl(Thread_Pool * const pub_this) noexcept(false)
    : Thread_Pool_Pimpl(pub_this, std::thread::hardware_concurrency())
    {
    }

    Thread_Pool_Pimpl(Thread_Pool * const pub_this, const int16_t num_threads) noexcept(false)
      : m_job_queue(Job_Queue::Create(pub_this))
    {
      m_worker_threads.reserve(num_threads);
      m_job_queues.reserve(num_threads);
      m_job_queues.emplace_back(std::make_pair(std::this_thread::get_id(), m_job_queue));
      for (int16_t num_threads_created = 1; num_threads_created < num_threads; ++num_threads_created) {
        std::shared_ptr<std::thread> new_thread;

        try {
          auto new_job_queue = Job_Queue::Create(pub_this);
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

    ~Thread_Pool_Pimpl() noexcept {
      finish_jobs();

#ifndef DISABLE_MULTITHREADING
      m_nonempty_job_queues.fetch_sub(1, std::memory_order_relaxed); // If everyone behaves, guaranteed to go negative to initiate termination in worker threads
      for (auto &worker : m_worker_threads)
        worker->join();
#endif
    }

    std::shared_ptr<Job_Queue> get_main_Job_Queue() const noexcept {
      return m_job_queue;
    }

#ifdef DISABLE_MULTITHREADING
    void finish_jobs() noexcept(false) {
      while (std::shared_ptr<Job> job = m_job_queue->try_take_one(true))
        job->execute();
    }
#else
    void finish_jobs() noexcept(false) {
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
          while (const std::shared_ptr<Job> job = (*jqt).second->try_take_one(true))
            job->execute();

          while (jqt != m_job_queues.end()) {
            if (const std::shared_ptr<Job> job = (*jqt).second->try_take_one(true)) {
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

    void worker_thread_work() noexcept {
      const auto thread_id = std::this_thread::get_id();
      while (!m_initialized.load(std::memory_order_acquire)) {
        if (m_failed_thread_id.load(std::memory_order_acquire) == thread_id)
          return;
      }

      std::vector<std::shared_ptr<Job_Queue>> job_queues;
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
          while (const std::shared_ptr<Job> job = (*jqt)->try_take_one(is_awake)) {
            is_awake = true;
            job->execute();
          }

          while(jqt != job_queues.end()) {
            if (const std::shared_ptr<Job> job = (*jqt)->try_take_one(is_awake)) {
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

    void worker_awakened() noexcept {
      m_awake_workers.fetch_add(1, std::memory_order_relaxed);
    }

    void job_queue_emptied() noexcept {
      m_nonempty_job_queues.fetch_sub(1, std::memory_order_relaxed);
    }

    void job_queue_nonemptied() noexcept {
      m_nonempty_job_queues.fetch_add(1, std::memory_order_relaxed);
    }
#endif

  private:
    std::shared_ptr<Job_Queue> m_job_queue;
#ifndef DISABLE_MULTITHREADING
    std::vector<std::shared_ptr<std::thread>> m_worker_threads;
    std::vector<std::pair<std::thread::id, std::shared_ptr<Job_Queue>>> m_job_queues;
    std::atomic_int16_t m_awake_workers = 0;
    std::atomic_int16_t m_nonempty_job_queues = 0;
    std::atomic_bool m_initialized = false;
    std::atomic<std::thread::id> m_failed_thread_id;
#endif
  };

#ifndef DISABLE_MULTITHREADING
  void worker(Thread_Pool_Pimpl * const thread_pool) noexcept {
    std::shared_ptr<Job_Queue> my_job_queue;
    std::vector<std::shared_ptr<Job_Queue>> other_job_queues;

    thread_pool->worker_thread_work();
  }
#endif

  const Thread_Pool_Pimpl * Thread_Pool::get_pimpl() const noexcept {
    return reinterpret_cast<const Thread_Pool_Pimpl *>(m_pimpl_storage);
  }

  Thread_Pool_Pimpl * Thread_Pool::get_pimpl() noexcept {
    return reinterpret_cast<Thread_Pool_Pimpl *>(m_pimpl_storage);
  }

  Thread_Pool::Thread_Pool() noexcept(false) {
    new (&m_pimpl_storage) Thread_Pool_Pimpl(this);
  }

  Thread_Pool::Thread_Pool(const int16_t num_threads) noexcept(false) {
    new (&m_pimpl_storage) Thread_Pool_Pimpl(this, num_threads);
  }

  Thread_Pool::~Thread_Pool() noexcept {
    static_assert(std::alignment_of<Thread_Pool_Pimpl>::value <= Thread_Pool::m_pimpl_align, "Thread_Pool::m_pimpl_align is too low.");
    ZENI_STATIC_WARNING(std::alignment_of<Thread_Pool_Pimpl>::value >= Thread_Pool::m_pimpl_align, "Thread_Pool::m_pimpl_align is too high.");

    static_assert(sizeof(Thread_Pool_Pimpl) <= sizeof(Thread_Pool::m_pimpl_storage), "Thread_Pool::m_pimpl_size too low.");
    ZENI_STATIC_WARNING(sizeof(Thread_Pool_Pimpl) >= sizeof(Thread_Pool::m_pimpl_storage), "Thread_Pool::m_pimpl_size too high.");

    get_pimpl()->~Thread_Pool_Pimpl();
  }

  std::shared_ptr<Job_Queue> Thread_Pool::get_main_Job_Queue() const noexcept {
    return get_pimpl()->get_main_Job_Queue();
  }

  void Thread_Pool::finish_jobs() noexcept(false) {
    get_pimpl()->finish_jobs();
  }

  void Thread_Pool::worker_awakened() noexcept {
#ifndef DISABLE_MULTITHREADING
    get_pimpl()->worker_awakened();
#endif
  }

  void Thread_Pool::job_queue_emptied() noexcept {
#ifndef DISABLE_MULTITHREADING
    get_pimpl()->job_queue_emptied();
#endif
  }

  void Thread_Pool::job_queue_nonemptied() noexcept {
#ifndef DISABLE_MULTITHREADING
    get_pimpl()->job_queue_nonemptied();
#endif
  }

}
