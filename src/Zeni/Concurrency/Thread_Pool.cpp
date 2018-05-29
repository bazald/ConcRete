#include "Zeni/Concurrency/Thread_Pool.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Utility.hpp"

#include <cassert>
#include <list>
#include <system_error>

#ifndef DISABLE_MULTITHREADING
#include <thread>
#endif

namespace Zeni::Concurrency {

  static void worker(const std::shared_ptr<Job_Queue> &job_queue) {
    for (;;) {
      const std::pair<std::shared_ptr<Job>, Job_Queue::Status> job_and_status = job_queue->take_one();
      if (job_and_status.second == Job_Queue::Status::SHUT_DOWN)
        break;
      assert(job_and_status.first);
      job_and_status.first->execute(*job_queue);
    }
  }

  class Thread_Pool_Pimpl {
    Thread_Pool_Pimpl(const Thread_Pool &) = delete;
    Thread_Pool_Pimpl & operator=(const Thread_Pool_Pimpl &) = delete;

  public:
    Thread_Pool_Pimpl()
#ifdef DISABLE_MULTITHREADING
      : Thread_Pool_Pimpl(0)
#else
      : Thread_Pool_Pimpl(std::thread::hardware_concurrency())
#endif
    {
    }

#ifdef DISABLE_MULTITHREADING
    Thread_Pool_Pimpl(const size_t)
#else
    Thread_Pool_Pimpl(const size_t num_threads)
#endif
      : m_job_queue(std::make_shared<Job_Queue>(0))
    {
#ifndef DISABLE_MULTITHREADING
      Job_Queue::Lock job_queue_lock(*m_job_queue);

      size_t num_threads_created;
      for (num_threads_created = 0; num_threads_created != num_threads; ++num_threads_created) {
        try {
          m_workers.emplace_back(std::thread(worker, m_job_queue));
        }
        catch (const std::system_error &) {
          break;
        }
      }

      m_job_queue->add_threads(num_threads_created);
#endif
    }

    ~Thread_Pool_Pimpl() {
      m_job_queue->finish();

#ifndef DISABLE_MULTITHREADING
      for (std::thread &worker : m_workers)
        worker.join();
#endif
    }

    std::shared_ptr<Job_Queue> get_Job_Queue() const {
      return m_job_queue;
    }

  private:
    std::shared_ptr<Job_Queue> m_job_queue;
#ifndef DISABLE_MULTITHREADING
    std::list<std::thread> m_workers;
#endif
  };

  const Thread_Pool_Pimpl * Thread_Pool::get_pimpl() const {
    return reinterpret_cast<const Thread_Pool_Pimpl *>(m_pimpl_storage);
  }

  Thread_Pool_Pimpl * Thread_Pool::get_pimpl() {
    return reinterpret_cast<Thread_Pool_Pimpl *>(m_pimpl_storage);
  }

  Thread_Pool::Thread_Pool() {
    new (&m_pimpl_storage) Thread_Pool_Pimpl;
  }

  Thread_Pool::Thread_Pool(const size_t num_threads) {
    new (&m_pimpl_storage) Thread_Pool_Pimpl(num_threads);
  }

  Thread_Pool::~Thread_Pool() {
    static_assert(std::alignment_of<Thread_Pool_Pimpl>::value <= Thread_Pool::m_pimpl_align, "Thread_Pool::m_pimpl_align is too low.");
    ZENI_STATIC_WARNING(std::alignment_of<Thread_Pool_Pimpl>::value >= Thread_Pool::m_pimpl_align, "Thread_Pool::m_pimpl_align is too high.");

    static_assert(sizeof(Thread_Pool_Pimpl) <= sizeof(Thread_Pool::m_pimpl_storage), "Thread_Pool::m_pimpl_size too low.");
    ZENI_STATIC_WARNING(sizeof(Thread_Pool_Pimpl) >= sizeof(Thread_Pool::m_pimpl_storage), "Thread_Pool::m_pimpl_size too high.");

    get_pimpl()->~Thread_Pool_Pimpl();
  }

  std::shared_ptr<Job_Queue> Thread_Pool::get_Job_Queue() const {
    return get_pimpl()->get_Job_Queue();
  }

}
