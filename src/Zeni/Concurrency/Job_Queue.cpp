#include "Zeni/Concurrency/Job_Queue.hpp"

#include "Zeni/Utility.hpp"

#ifndef DISABLE_MULTITHREADING
#include <condition_variable>
#include <thread>
#include <mutex>
#endif

#include <queue>

namespace Zeni::Concurrency {

  Job_Queue::Job_Queue_Must_Be_Running::Job_Queue_Must_Be_Running()
    : runtime_error("Invalid operation attempted for a Job_Queue that has shut down or is in the process of being shut down.")
  {
  }

  Job_Queue::Job_Queue_Must_Not_Be_Shut_Down::Job_Queue_Must_Not_Be_Shut_Down()
    : runtime_error("Invalid operation attempted for a Job_Queue that has already been shut down.")
  {
  }

  class Job_Queue_Pimpl {
    friend class Job_Queue_Lock_Pimpl;

  public:
    Job_Queue_Pimpl()
#ifdef DISABLE_MULTITHREADING
      : m_dormant_max(0)
#else
      : m_dormant_max(std::thread::hardware_concurrency() > 1 ? std::thread::hardware_concurrency() : 0)
#endif
    {
    }

#ifdef DISABLE_MULTITHREADING
    Job_Queue_Pimpl(const size_t)
      : m_dormant_max(0)
#else
    Job_Queue_Pimpl(const size_t num_threads)
      : m_dormant_max(num_threads > 1 ? num_threads : 0)
#endif
    {
    }

    size_t num_threads() const {
      return m_dormant_max;
    }

    void add_threads(const size_t threads) {
      m_dormant_max += threads;
    }

    void finish() {
#ifndef DISABLE_MULTITHREADING
      std::lock_guard<std::mutex> mutex_lock(m_mutex);
#endif

      if (m_status != Job_Queue::Status::RUNNING)
        throw Job_Queue::Job_Queue_Must_Be_Running();

      m_status = Job_Queue::Status::SHUTTING_DOWN;
#ifndef DISABLE_MULTITHREADING
      m_non_empty.notify_one();
#endif

      //std::ostringstream oss;
      //oss << "Number of Jobs remaining: " << m_jobs.size() << std::endl;
      //std::cerr << oss.str();
    }

    void wait_for_completion(Job_Queue * const pub_this) {
#ifndef DISABLE_MULTITHREADING
      if (m_dormant_max) {
        std::unique_lock<std::mutex> mutex_lock(m_mutex);
        while (!m_jobs.empty() || m_dormant != m_dormant_max)
          m_empty.wait(mutex_lock);
      }
      else
#endif
      {
        while (!m_jobs.empty()) {
          while (!m_jobs.front().empty()) {
            const auto job = m_jobs.front().back();
            m_jobs.front().pop_back();
            job->execute(*pub_this);
          }
          m_jobs.pop();
        }
      }
    }

    std::pair<std::shared_ptr<Job>, Job_Queue::Status> take_one() {
#ifndef DISABLE_MULTITHREADING
      std::unique_lock<std::mutex> mutex_lock(m_mutex);
#endif

      if (m_jobs.empty()) {
        ++m_dormant;
        do {
          if (m_status == Job_Queue::Status::SHUTTING_DOWN && m_dormant == m_dormant_max) {
            m_status = Job_Queue::Status::SHUT_DOWN;
#ifndef DISABLE_MULTITHREADING
            m_non_empty.notify_all();
#endif
          }

          if (m_status == Job_Queue::Status::SHUT_DOWN)
            return std::make_pair(std::shared_ptr<Job>(nullptr), m_status);

#ifndef DISABLE_MULTITHREADING
          if (m_dormant == m_dormant_max)
            m_empty.notify_all();

          m_non_empty.wait(mutex_lock);
#endif
        } while (m_jobs.empty());
        --m_dormant;
      }

      const std::shared_ptr<Job> job = m_jobs.front().back();
      m_jobs.front().pop_back();

      if (m_jobs.front().empty())
        m_jobs.pop();

      return std::make_pair(job, m_status);
    }

    void give_one(const std::shared_ptr<Job> job) {
#ifndef DISABLE_MULTITHREADING
      std::unique_lock<std::mutex> mutex_lock(m_mutex);
#endif

      if (m_status == Job_Queue::Status::SHUT_DOWN)
        throw Job_Queue::Job_Queue_Must_Not_Be_Shut_Down();

      m_jobs.emplace(1, job);
#ifndef DISABLE_MULTITHREADING
      m_non_empty.notify_one();
#endif
    }

    void give_many(std::vector<std::shared_ptr<Job>> &&jobs) {
      if (jobs.empty())
        return;

#ifndef DISABLE_MULTITHREADING
      std::unique_lock<std::mutex> mutex_lock(m_mutex);
#endif

      if (m_status == Job_Queue::Status::SHUT_DOWN)
        throw Job_Queue::Job_Queue_Must_Not_Be_Shut_Down();

      m_jobs.emplace(std::move(jobs));
#ifndef DISABLE_MULTITHREADING
      m_non_empty.notify_all();
#endif
    }

    void give_many(const std::vector<std::shared_ptr<Job>> &jobs) {
      if (jobs.empty())
        return;

#ifndef DISABLE_MULTITHREADING
      std::unique_lock<std::mutex> mutex_lock(m_mutex);
#endif

      if (m_status == Job_Queue::Status::SHUT_DOWN)
        throw Job_Queue::Job_Queue_Must_Not_Be_Shut_Down();

      m_jobs.emplace(jobs);
#ifndef DISABLE_MULTITHREADING
      m_non_empty.notify_all();
#endif
    }

  private:
#ifndef DISABLE_MULTITHREADING
    std::mutex m_mutex;
    std::condition_variable m_empty;
    std::condition_variable m_non_empty;
#endif
    std::queue<std::vector<std::shared_ptr<Job>>> m_jobs;
    Job_Queue::Status m_status = Job_Queue::Status::RUNNING;
    size_t m_dormant = 0;
    size_t m_dormant_max;
  };

  const Job_Queue_Pimpl * Job_Queue::get_pimpl() const {
    return reinterpret_cast<const Job_Queue_Pimpl *>(m_pimpl_storage);
  }

  Job_Queue_Pimpl * Job_Queue::get_pimpl() {
    return reinterpret_cast<Job_Queue_Pimpl *>(m_pimpl_storage);
  }

  class Job_Queue_Lock_Pimpl {
    Job_Queue_Lock_Pimpl(const Job_Queue_Lock_Pimpl &) = delete;
    Job_Queue_Lock_Pimpl & operator=(const Job_Queue_Lock_Pimpl &) = delete;

  public:
#ifdef DISABLE_MULTITHREADING
    Job_Queue_Lock_Pimpl(Job_Queue &)
#else
    Job_Queue_Lock_Pimpl(Job_Queue &job_queue)
      : m_lock(job_queue.get_pimpl()->m_mutex)
#endif
    {
    }

#ifndef DISABLE_MULTITHREADING
  private:
    std::lock_guard<std::mutex> m_lock;
#endif
  };

  const Job_Queue_Lock_Pimpl * Job_Queue::Lock::get_pimpl() const {
    return reinterpret_cast<const Job_Queue_Lock_Pimpl *>(m_pimpl_storage);
  }
  Job_Queue_Lock_Pimpl * Job_Queue::Lock::get_pimpl() {
    return reinterpret_cast<Job_Queue_Lock_Pimpl *>(m_pimpl_storage);
  }

  Job_Queue::Lock::Lock(Job_Queue &job_queue) {
    new (&m_pimpl_storage) Job_Queue_Lock_Pimpl(job_queue);
  }

  Job_Queue::Lock::~Lock() {
    static_assert(std::alignment_of<Job_Queue_Lock_Pimpl>::value <= Job_Queue::Lock::m_pimpl_align, "Job_Queue::Lock::m_pimpl_align is too low.");
    ZENI_STATIC_WARNING(std::alignment_of<Job_Queue_Lock_Pimpl>::value >= Job_Queue::Lock::m_pimpl_align, "Job_Queue::Lock::m_pimpl_align is too high.");

    static_assert(sizeof(Job_Queue_Lock_Pimpl) <= sizeof(Job_Queue::Lock::m_pimpl_storage), "Job_Queue::Lock::m_pimpl_size too low.");
    ZENI_STATIC_WARNING(sizeof(Job_Queue_Lock_Pimpl) >= sizeof(Job_Queue::Lock::m_pimpl_storage), "Job_Queue::Lock::m_pimpl_size too high.");

    get_pimpl()->~Job_Queue_Lock_Pimpl();
  }

  Job_Queue::Job_Queue() {
    new (&m_pimpl_storage) Job_Queue_Pimpl;
  }

  Job_Queue::Job_Queue(const size_t num_threads) {
    new (&m_pimpl_storage) Job_Queue_Pimpl(num_threads);
  }

  Job_Queue::~Job_Queue() {
    static_assert(std::alignment_of<Job_Queue_Pimpl>::value <= Job_Queue::m_pimpl_align, "Job_Queue::m_pimpl_align is too low.");
    ZENI_STATIC_WARNING(std::alignment_of<Job_Queue_Pimpl>::value >= Job_Queue::m_pimpl_align, "Job_Queue::m_pimpl_align is too high.");

    static_assert(sizeof(Job_Queue_Pimpl) <= sizeof(Job_Queue::m_pimpl_storage), "Job_Queue::m_pimpl_size too low.");
    ZENI_STATIC_WARNING(sizeof(Job_Queue_Pimpl) >= sizeof(Job_Queue::m_pimpl_storage), "Job_Queue::m_pimpl_size too high.");
    
    get_pimpl()->~Job_Queue_Pimpl();
  }

  size_t Job_Queue::num_threads() const {
    return get_pimpl()->num_threads();
  }

  void Job_Queue::add_threads(const size_t threads) {
    get_pimpl()->add_threads(threads);
  }

  void Job_Queue::finish() {
    return get_pimpl()->finish();
  }

  void Job_Queue::wait_for_completion() {
    return get_pimpl()->wait_for_completion(this);
  }

  std::pair<std::shared_ptr<Job>, Job_Queue::Status> Job_Queue::take_one() {
    return get_pimpl()->take_one();
  }

  void Job_Queue::give_one(const std::shared_ptr<Job> job) {
    return get_pimpl()->give_one(job);
  }

  void Job_Queue::give_many(std::vector<std::shared_ptr<Job>> &&jobs) {
    return get_pimpl()->give_many(std::move(jobs));
  }

  void Job_Queue::give_many(const std::vector<std::shared_ptr<Job>> &jobs) {
    return get_pimpl()->give_many(jobs);
  }

}
