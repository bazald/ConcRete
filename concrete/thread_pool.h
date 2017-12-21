#ifndef ZENI_THREAD_POOL
#define ZENI_THREAD_POOL

#include "job_queue.h"
#include <list>

class Thread_Pool {
public:
  /// Initializes a thread pool with a number of threads equal to what is specified in the Job_Queue
  Thread_Pool(const Job_Queue::Ptr &job_queue);
  ~Thread_Pool();

  Job_Queue::Ptr get_queue() const;

private:
  Job_Queue::Ptr m_job_queue;
  std::list<std::thread> m_workers;
};

#endif
