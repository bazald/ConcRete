#include "Zeni/Concurrency/Job_Queue.h"

#include <cassert>
#include <iostream>
#include <sstream>

namespace Zeni {

  namespace Concurrency {

    Job_Queue::Job_Queue(const size_t &num_threads)
      : m_dormant_max(num_threads)
    {
    }

    size_t Job_Queue::num_threads() const {
      return m_dormant_max;
    }

    void Job_Queue::finish() {
      std::unique_lock<std::mutex> mutex_lock(m_mutex);

      if (m_status != Status::RUNNING)
        throw Job_Queue_Must_Be_Running();

      m_status = Status::SHUTTING_DOWN;
      m_non_empty.notify_one();

      std::ostringstream oss;
      oss << "Number of Jobs remaining: " << m_jobs.size() << std::endl;
      std::cerr << oss.str();
    }

    void Job_Queue::wait_for_completion() {
      std::unique_lock<std::mutex> mutex_lock(m_mutex);
      while (!m_jobs.empty() || m_dormant != m_dormant_max)
        m_empty.wait(mutex_lock);
    }

    std::pair<Job::Ptr, Job_Queue::Status> Job_Queue::take() {
      std::unique_lock<std::mutex> mutex_lock(m_mutex);

      if (m_jobs.empty()) {
        ++m_dormant;
        do {
          if (m_status == Status::SHUTTING_DOWN && m_dormant == m_dormant_max) {
            m_status = Status::SHUT_DOWN;
            m_non_empty.notify_all();
          }

          if (m_status == Status::SHUT_DOWN)
            return std::make_pair(Job::Ptr(nullptr), m_status);

          if (m_dormant == m_dormant_max)
            m_empty.notify_all();

          m_non_empty.wait(mutex_lock);
        } while (m_jobs.empty());
        --m_dormant;
      }

      const Job::Ptr job = m_jobs.front();
      m_jobs.pop();
      return std::make_pair(job, m_status);
    }

    void Job_Queue::give(const Job::Ptr &job) {
      std::unique_lock<std::mutex> mutex_lock(m_mutex);

      if (m_status == Status::SHUT_DOWN)
        throw Job_Queue_Must_Not_Be_Shut_Down();

      m_jobs.push(job);
      m_non_empty.notify_one();
    }

    void Job_Queue::give(Job::Ptr &&job) {
      std::unique_lock<std::mutex> mutex_lock(m_mutex);

      if (m_status == Status::SHUT_DOWN)
        throw Job_Queue_Must_Not_Be_Shut_Down();

      m_jobs.push(std::move(job));
      m_non_empty.notify_one();
    }

  }

}
