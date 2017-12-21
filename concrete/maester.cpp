#include "maester.h"

#include "raven.h"

Maester::Maester(const Job_Queue::Ptr &job_queue)
  : m_job_queue(job_queue)
{
}

Maester::Maester(Job_Queue::Ptr &&job_queue)
  : m_job_queue(std::move(job_queue))
{
}

Job_Queue::Ptr Maester::get_job_queue() const {
  return m_job_queue;
}