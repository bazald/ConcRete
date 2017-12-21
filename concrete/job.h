#ifndef ZENI_JOB_H
#define ZENI_JOB_H

#include <memory>

/// A Job virtual base class
class Job : public std::enable_shared_from_this<Job> {
public:
  typedef std::shared_ptr<Job> Ptr;

  /// The function that gets called by whichever worker pulls this Job off of the Job_Queue
  virtual void execute() = 0;
};

#endif
