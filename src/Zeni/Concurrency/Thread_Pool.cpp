#include "Zeni/Concurrency/Thread_Pool.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include <cassert>
#include <list>
#include <thread>

namespace Zeni {

  namespace Concurrency {

    static void worker(const std::shared_ptr<Job_Queue> &job_queue) {
      for (;;) {
        const std::pair<std::shared_ptr<Job>, Job_Queue::Status> job_and_status = job_queue->take();
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
      Thread_Pool_Pimpl(const std::shared_ptr<Job_Queue> &job_queue)
        : m_job_queue(job_queue)
      {
        for (size_t i = 0; i != job_queue->num_threads(); ++i)
          m_workers.emplace_back(std::thread(worker, job_queue));
      }

      ~Thread_Pool_Pimpl() {
        m_job_queue->finish();

        for (std::thread &worker : m_workers)
          worker.join();
      }

      std::shared_ptr<Job_Queue> get_queue() const {
        return m_job_queue;
      }

    private:
      std::shared_ptr<Job_Queue> m_job_queue;
      std::list<std::thread> m_workers;
    };

    Thread_Pool::Thread_Pool(const std::shared_ptr<Job_Queue> &job_queue)
      : m_impl(new Thread_Pool_Pimpl(job_queue))
    {
    }

    Thread_Pool::~Thread_Pool() {
      delete m_impl;
    }

    std::shared_ptr<Job_Queue> Thread_Pool::get_queue() const {
      return m_impl->get_queue();
    }

  }

}
