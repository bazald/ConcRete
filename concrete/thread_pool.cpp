#include "thread_pool.h"

#include "job_queue.h"
#include <cassert>

static void worker(const Job_Queue::Ptr &job_queue) {
  for(;;) {
    const std::pair<Job::Ptr, Job_Queue::Status> job_and_status = job_queue->take();
    if(job_and_status.second == Job_Queue::Status::SHUT_DOWN)
      break;
    assert(job_and_status.first);
    job_and_status.first->execute();
  }
}

Thread_Pool::Thread_Pool(const Job_Queue::Ptr &job_queue)
  : m_job_queue(job_queue)
{
  for(size_t i = 0; i != job_queue->num_threads(); ++i)
    m_workers.emplace_back(std::thread(worker, job_queue));
}

Thread_Pool::~Thread_Pool() {
  m_job_queue->finish();

  for(std::thread &worker : m_workers)
    worker.join();
}

Job_Queue::Ptr Thread_Pool::get_queue() const {
  return m_job_queue;
}
