#ifndef ZENI_CONCURRENCY_JOB_QUEUE_H
#define ZENI_CONCURRENCY_JOB_QUEUE_H

#include "Job.h"

#include <stdexcept>

namespace Zeni {

  namespace Concurrency {

    class Job_Queue_Pimpl;

    class ZENI_CONCURRENCY_LINKAGE Job_Queue {
      Job_Queue(const Job_Queue &) = delete;
      Job_Queue operator=(const Job_Queue &) = delete;

    public:
      enum class Status { RUNNING, SHUTTING_DOWN, SHUT_DOWN };

      class Job_Queue_Must_Be_Running : public std::runtime_error {
      public:
        Job_Queue_Must_Be_Running()
          : runtime_error("Invalid operation attempted for a Job_Queue that has shut down or is in the process of being shut down.")
        {
        }
      };

      class Job_Queue_Must_Not_Be_Shut_Down : public std::runtime_error {
      public:
        Job_Queue_Must_Not_Be_Shut_Down()
          : runtime_error("Invalid operation attempted for a Job_Queue that has already been shut down.")
        {
        }
      };

      Job_Queue(const size_t &num_threads);
      ~Job_Queue();

      /// Get the number of threads.
      size_t num_threads() const;

      /// Change status to SHUTTING_DOWN and wake a thread in case all are waiting on non-empty. Can throw Job_Queue_Must_Be_Running.
      void finish();

      /// Wait for the number of jobs to go to 0 and all worker threads to go dormant. i.e. quiescence
      void wait_for_completion();

      /// Take a Job off the queue. Will be null if and only if SHUT_DOWN.
      std::pair<std::shared_ptr<Job>, Status> take();

      /// Give the queue a new Job. Can throw Job_Queue_Must_Not_Be_Shut_Down.
      void give(const std::shared_ptr<Job> &job);
      /// Give the queue a new Job. Can throw Job_Queue_Must_Not_Be_Shut_Down.
      void give(std::shared_ptr<Job> &&job);

    private:
      Job_Queue_Pimpl * const m_impl;
    };

  }

}

#endif
