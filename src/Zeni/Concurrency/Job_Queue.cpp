#include "Zeni/Concurrency/Job_Queue.hpp"

#include <condition_variable>
#include <thread>
#include <mutex>
#include <queue>

#include <cassert>
#include <sstream>

namespace Zeni {

  namespace Concurrency {

    class Job_Queue_Pimpl {
      friend class Job_Queue_Lock_Pimpl;

    public:
      Job_Queue_Pimpl()
        : Job_Queue_Pimpl(std::thread::hardware_concurrency())
      {
      }

      Job_Queue_Pimpl(const size_t &num_threads)
        : m_dormant_max(num_threads)
      {
      }

      size_t num_threads() const {
        return m_dormant_max;
      }

      void add_threads(const size_t &threads) {
        m_dormant_max += threads;
      }

      void finish() {
        std::lock_guard<std::mutex> mutex_lock(m_mutex);

        if (m_status != Job_Queue::Status::RUNNING)
          throw Job_Queue::Job_Queue_Must_Be_Running();

        m_status = Job_Queue::Status::SHUTTING_DOWN;
        m_non_empty.notify_one();

        //std::ostringstream oss;
        //oss << "Number of Jobs remaining: " << m_jobs.size() << std::endl;
        //std::cerr << oss.str();
      }

      void wait_for_completion() {
        std::unique_lock<std::mutex> mutex_lock(m_mutex);
        while (!m_jobs.empty() || m_dormant != m_dormant_max)
          m_empty.wait(mutex_lock);
      }

      std::pair<std::shared_ptr<Job>, Job_Queue::Status> take() {
        std::unique_lock<std::mutex> mutex_lock(m_mutex);

        if (m_jobs.empty()) {
          ++m_dormant;
          do {
            if (m_status == Job_Queue::Status::SHUTTING_DOWN && m_dormant == m_dormant_max) {
              m_status = Job_Queue::Status::SHUT_DOWN;
              m_non_empty.notify_all();
            }

            if (m_status == Job_Queue::Status::SHUT_DOWN)
              return std::make_pair(std::shared_ptr<Job>(nullptr), m_status);

            if (m_dormant == m_dormant_max)
              m_empty.notify_all();

            m_non_empty.wait(mutex_lock);
          } while (m_jobs.empty());
          --m_dormant;
        }

        const std::shared_ptr<Job> job = m_jobs.front();
        m_jobs.pop();
        return std::make_pair(job, m_status);
      }

      void give(Job_Queue * const pub_this, const std::shared_ptr<Job> &job) {
        std::unique_lock<std::mutex> mutex_lock(m_mutex);

        if (m_status == Job_Queue::Status::SHUT_DOWN)
          throw Job_Queue::Job_Queue_Must_Not_Be_Shut_Down();

        if (m_dormant_max) {
          m_jobs.push(job);
          m_non_empty.notify_one();
        }
        else {
          // Single-threaded mode
          mutex_lock.release();
          m_mutex.unlock();
          job->execute(*pub_this);
        }
      }

      void give(Job_Queue * const pub_this, std::shared_ptr<Job> &&job) {
        std::unique_lock<std::mutex> mutex_lock(m_mutex);

        if (m_status == Job_Queue::Status::SHUT_DOWN)
          throw Job_Queue::Job_Queue_Must_Not_Be_Shut_Down();

        if (m_dormant_max) {
          m_jobs.push(std::move(job));
          m_non_empty.notify_one();
        }
        else {
          // Single-threaded mode
          mutex_lock.release();
          m_mutex.unlock();
          job->execute(*pub_this);
        }
      }

    private:
      std::mutex m_mutex;
      std::queue<std::shared_ptr<Job>> m_jobs;
      std::condition_variable m_empty;
      std::condition_variable m_non_empty;
      Job_Queue::Status m_status = Job_Queue::Status::RUNNING;
      size_t m_dormant = 0;
      size_t m_dormant_max;
    };

    class Job_Queue_Lock_Pimpl {
      Job_Queue_Lock_Pimpl(const Job_Queue_Lock_Pimpl &) = delete;
      Job_Queue_Lock_Pimpl & operator=(const Job_Queue_Lock_Pimpl &) = delete;

    public:
      Job_Queue_Lock_Pimpl(Job_Queue &job_queue)
        : m_lock(job_queue.m_impl->m_mutex)
      {
      }

    private:
      std::lock_guard<std::mutex> m_lock;
    };

    Job_Queue::Lock::Lock(Job_Queue &job_queue)
      : m_impl(new Job_Queue_Lock_Pimpl(job_queue))
    {
    }

    Job_Queue::Lock::~Lock() {
      delete m_impl;
    }

    Job_Queue::Job_Queue()
      : m_impl(new Job_Queue_Pimpl)
    {
    }

    Job_Queue::Job_Queue(const size_t &num_threads)
      : m_impl(new Job_Queue_Pimpl(num_threads))
    {
    }

    Job_Queue::~Job_Queue()
    {
      delete m_impl;
    }

    size_t Job_Queue::num_threads() const {
      return m_impl->num_threads();
    }

    void Job_Queue::add_threads(const size_t &threads) {
      m_impl->add_threads(threads);
    }

    void Job_Queue::finish() {
      return m_impl->finish();
    }

    void Job_Queue::wait_for_completion() {
      return m_impl->wait_for_completion();
    }

    std::pair<std::shared_ptr<Job>, Job_Queue::Status> Job_Queue::take() {
      return m_impl->take();
    }

    void Job_Queue::give(const std::shared_ptr<Job> &job) {
      return m_impl->give(this, job);
    }

    void Job_Queue::give(std::shared_ptr<Job> &&job) {
      return m_impl->give(this, std::move(job));
    }

  }

}
