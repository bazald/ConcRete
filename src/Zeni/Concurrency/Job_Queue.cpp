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

      std::pair<std::shared_ptr<Job>, Job_Queue::Status> take_one() {
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

        const std::shared_ptr<Job> job = m_jobs.front().back();
        m_jobs.front().pop_back();

        if (m_jobs.front().empty())
          m_jobs.pop();

        return std::make_pair(job, m_status);
      }

      void give_one(Job_Queue * const pub_this, const std::shared_ptr<Job> &job) {
        std::unique_lock<std::mutex> mutex_lock(m_mutex);

        if (m_status == Job_Queue::Status::SHUT_DOWN)
          throw Job_Queue::Job_Queue_Must_Not_Be_Shut_Down();

        if (m_dormant_max) {
          m_jobs.emplace(1, job);
          m_non_empty.notify_one();
        }
        else {
          // Single-threaded mode
          mutex_lock.release();
          m_mutex.unlock();
          job->execute(*pub_this);
        }
      }

      void give_many(Job_Queue * const pub_this, std::vector<std::shared_ptr<Job>> &&jobs) {
        if (jobs.empty())
          return;

        std::unique_lock<std::mutex> mutex_lock(m_mutex);

        if (m_status == Job_Queue::Status::SHUT_DOWN)
          throw Job_Queue::Job_Queue_Must_Not_Be_Shut_Down();

        if (m_dormant_max) {
          m_jobs.emplace(std::move(jobs));
          m_non_empty.notify_all();
        }
        else {
          // Single-threaded mode
          mutex_lock.release();
          m_mutex.unlock();
          for(auto job : jobs)
            job->execute(*pub_this);
        }
      }

    private:
      std::mutex m_mutex;
      std::queue<std::vector<std::shared_ptr<Job>>> m_jobs;
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

    std::pair<std::shared_ptr<Job>, Job_Queue::Status> Job_Queue::take_one() {
      return m_impl->take_one();
    }

    void Job_Queue::give_one(const std::shared_ptr<Job> &job) {
      return m_impl->give_one(this, job);
    }

    void Job_Queue::give_many(std::vector<std::shared_ptr<Job>> &&jobs) {
      return m_impl->give_many(this, std::move(jobs));
    }

  }

}
