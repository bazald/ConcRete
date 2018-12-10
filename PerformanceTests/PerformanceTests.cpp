#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>
#endif

#include "Zeni/Concurrency/Container/Antiable_Hash_Trie.hpp"
#include "Zeni/Concurrency/Container/Hash_Trie.hpp"
#include "Zeni/Concurrency/Container/Hash_Trie_2.hpp"
#include "Zeni/Concurrency/Container/Hash_Trie_S2.hpp"
#include "Zeni/Concurrency/Container/Positive_Hash_Trie.hpp"
#include "Zeni/Concurrency/Container/Super_Hash_Trie.hpp"
#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Concurrency/Message.hpp"
#include "Zeni/Concurrency/Mutex.hpp"
#include "Zeni/Concurrency/Recipient.hpp"
#include "Zeni/Concurrency/Worker_Threads.hpp"

#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Node_Filter_1.hpp"
#include "Zeni/Rete/Node_Filter_2.hpp"
#include "Zeni/Rete/Node_Join.hpp"
#include "Zeni/Rete/Node_Key.hpp"
#include "Zeni/Rete/Node_Predicate.hpp"
#include "Zeni/Rete/Parser.hpp"
#include "Zeni/Rete/Variable_Indices.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <iostream>
#include <list>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>

template<typename TimeT = std::chrono::milliseconds>
struct measure
{
  template<typename F, typename ...Args>
  static typename TimeT::rep execution(F&& func, Args&&... args)
  {
    auto start = std::chrono::steady_clock::now();
    std::forward<decltype(func)>(func)(std::forward<Args>(args)...);
    auto duration = std::chrono::duration_cast<TimeT>
      (std::chrono::steady_clock::now() - start);
    return duration.count();
  }
};

static void test_Job_Performance(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_Memory_Performance(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_Reclamation_Performance(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);

static int16_t g_num_cores = 0;

int main(int argc, char **argv)
{
#ifdef _MSC_VER
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  //_CrtSetBreakAlloc(4479);
#endif

  if (argc > 2 || (argc == 2 && !std::strcmp(argv[1], "--help"))) {
    std::cerr << "Usage: " << argv[0] << " [cores=" << std::thread::hardware_concurrency() << ']' << std::endl;
    return 1;
  }
  else if (argc == 2)
    g_num_cores = int16_t(std::atoi(argv[1]));
  else
    g_num_cores = std::thread::hardware_concurrency();

  const auto worker_threads = Zeni::Concurrency::Worker_Threads::Create(g_num_cores);
  const auto job_queue = worker_threads->get_main_Job_Queue();

  std::cerr << "Job Performance: " << measure<>::execution(test_Job_Performance, worker_threads, job_queue) / 1000.0 << 's' << std::endl;

  std::cerr << "Memory Performance: " << measure<>::execution(test_Memory_Performance, worker_threads, job_queue) / 1000.0 << 's' << std::endl;

  std::cerr << "Reclamation Performance: " << measure<>::execution(test_Reclamation_Performance, worker_threads, job_queue) / 1000.0 << 's' << std::endl;

  return 0;
}

class Simple_Job : public Zeni::Concurrency::Job {
  Simple_Job(const Simple_Job &) = delete;
  Simple_Job & operator =(const Simple_Job &) = delete;

public:
  Simple_Job(const size_t num_successors_) : num_successors(num_successors_) {}

  void execute() noexcept {
    volatile int a = 0;
    for (int i = 0; i != 1000000; ++i)
      a += i;

    if (num_successors)
      get_Job_Queue()->give_one(std::make_shared<Simple_Job>(num_successors - 1));
  }

  const size_t num_successors;
};

void test_Job_Performance(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue)
{
  for (int i = 0; i != 64; ++i)
    job_queue->give_one(std::make_shared<Simple_Job>(64));
  worker_threads->finish_jobs();
}

class Memory_Job : public Zeni::Concurrency::Job {
  Memory_Job(const Memory_Job &) = delete;
  Memory_Job & operator =(const Memory_Job &) = delete;

public:
  Memory_Job(const size_t num_successors_) : num_successors(num_successors_) {}

  void execute() noexcept {
    volatile int a = 0;
    for (int i = 0; i != 100000; ++i) {
      const auto ii = std::make_shared<int>(i);
      a += *ii;
    }

    if (num_successors)
      get_Job_Queue()->give_one(std::make_shared<Simple_Job>(num_successors - 1));
  }

  const size_t num_successors;
};

void test_Memory_Performance(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue)
{
  for (int i = 0; i != 64; ++i)
    job_queue->give_one(std::make_shared<Memory_Job>(64));
  worker_threads->finish_jobs();
}

class Reclamation_Job : public Zeni::Concurrency::Job {
  Reclamation_Job(const Reclamation_Job &) = delete;
  Reclamation_Job & operator =(const Reclamation_Job &) = delete;

  struct Int : public Zeni::Concurrency::Enable_Intrusive_Sharing {
    int value;
  };

public:
  Reclamation_Job(const size_t num_successors_) : num_successors(num_successors_) {}

  void execute() noexcept {
    volatile int a = 0;
    for (int i = 0; i != 100000; ++i) {
      const Zeni::Concurrency::Intrusive_Shared_Ptr<Int>::Lock ii = new Int();
      a += ii->value;
    }

    if (num_successors)
      get_Job_Queue()->give_one(std::make_shared<Simple_Job>(num_successors - 1));
  }

  const size_t num_successors;
};

void test_Reclamation_Performance(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue)
{
  for (int i = 0; i != 64; ++i)
    job_queue->give_one(std::make_shared<Reclamation_Job>(64));
  worker_threads->finish_jobs();
}
