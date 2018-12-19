#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>
#endif

#include "Zeni/Concurrency/Container/Antiable_Hash_Trie.hpp"
#include "Zeni/Concurrency/Container/Ctrie.hpp"
#include "Zeni/Concurrency/Container/Hash_Trie.hpp"
#include "Zeni/Concurrency/Container/Hash_Trie_2.hpp"
#include "Zeni/Concurrency/Container/Hash_Trie_S2.hpp"
#include "Zeni/Concurrency/Container/Intrusive_Stack.hpp"
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
static void test_Hash_Trie_Performance(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_Ctrie_Performance(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_Reclamation_Performance(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_SmarterRec_Performance(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);

static int16_t g_num_cores = 0;

int main(int argc, char **argv)
{
#ifdef _MSC_VER
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  //_CrtSetBreakAlloc(201);
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

  //std::cerr << "Job Performance: " << measure<>::execution(test_Job_Performance, worker_threads, job_queue) / 1000.0 << 's' << std::endl;

  //std::cerr << "Memory Performance: " << measure<>::execution(test_Memory_Performance, worker_threads, job_queue) / 1000.0 << 's' << std::endl;

  //std::cerr << "Hash Trie Performance: " << measure<>::execution(test_Hash_Trie_Performance, worker_threads, job_queue) / 1000.0 << 's' << std::endl;

  std::cerr << "Ctrie Performance: " << measure<>::execution(test_Ctrie_Performance, worker_threads, job_queue) / 1000.0 << 's' << std::endl;

  //std::cerr << "Reclamation Performance: " << measure<>::execution(test_Reclamation_Performance, worker_threads, job_queue) / 1000.0 << 's' << std::endl;

  //std::cerr << "SmarterRec Performance: " << measure<>::execution(test_SmarterRec_Performance, worker_threads, job_queue) / 1000.0 << 's' << std::endl;

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
    job_queue->give_one(std::make_shared<Simple_Job>(63));
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
      get_Job_Queue()->give_one(std::make_shared<Memory_Job>(num_successors - 1));
  }

  const size_t num_successors;
};

void test_Memory_Performance(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue)
{
  for (int i = 0; i != 64; ++i)
    job_queue->give_one(std::make_shared<Memory_Job>(63));
  worker_threads->finish_jobs();
}

class Hash_Trie_Job : public Zeni::Concurrency::Job {
  Hash_Trie_Job(const Hash_Trie_Job &) = delete;
  Hash_Trie_Job & operator =(const Hash_Trie_Job &) = delete;

  struct Int : public Zeni::Concurrency::Enable_Intrusive_Sharing {
    int value;
  };

public:
  Hash_Trie_Job(const std::shared_ptr<Zeni::Concurrency::Hash_Trie<int>> hash_trie_, const size_t num_successors_) : hash_trie(hash_trie_), num_successors(num_successors_) {}

  void execute() noexcept {
    for (int i = 0; i != 1000; ++i)
      hash_trie->insert(i);
    for (int i = 0; i != 1000; ++i)
      hash_trie->erase(i);

    if (num_successors)
      get_Job_Queue()->give_one(std::make_shared<Hash_Trie_Job>(hash_trie, num_successors - 1));
  }

  const std::shared_ptr<Zeni::Concurrency::Hash_Trie<int>> hash_trie;
  const size_t num_successors;
};

void test_Hash_Trie_Performance(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue)
{
  const auto hash_trie = std::make_shared<Zeni::Concurrency::Hash_Trie<int>>();
  for (int i = 0; i != 64; ++i)
    job_queue->give_one(std::make_shared<Hash_Trie_Job>(hash_trie, 63));
  worker_threads->finish_jobs();
}

template <typename TYPE, size_t MOD>
struct Hash_Mod {
  size_t operator()(const TYPE &value) const {
    return std::hash<TYPE>()(value); // % MOD;
  }
};

class Ctrie_Job : public Zeni::Concurrency::Job {
  Ctrie_Job(const Ctrie_Job &) = delete;
  Ctrie_Job & operator =(const Ctrie_Job &) = delete;

  struct Int : public Zeni::Concurrency::Enable_Intrusive_Sharing {
    int value;
  };

public:
  Ctrie_Job(const std::shared_ptr<Zeni::Concurrency::Ctrie<int, Hash_Mod<int, 10>, std::equal_to<int>, uint8_t>> hash_trie_, const size_t num_successors_) : hash_trie(hash_trie_), num_successors(num_successors_) {}

  void execute() noexcept {
    for (int i = 0; i != 1000; ++i) {
      hash_trie->insert(i);
      hash_trie->snapshot();
    }
    for (int i = 0; i != 1000; ++i) {
      hash_trie->erase(i);
      hash_trie->snapshot();
    }

    if (num_successors)
      get_Job_Queue()->give_one(std::make_shared<Ctrie_Job>(hash_trie, num_successors - 1));
  }

  const std::shared_ptr<Zeni::Concurrency::Ctrie<int, Hash_Mod<int, 10>, std::equal_to<int>, uint8_t>> hash_trie;
  const size_t num_successors;
};

void test_Ctrie_Performance(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue)
{
  const auto hash_trie = std::allocate_shared<Zeni::Concurrency::Ctrie<int, Hash_Mod<int, 10>, std::equal_to<int>, uint8_t>>(Zeni::Concurrency::Ctrie<int>::Allocator());
  for (int i = 0; i != 64; ++i)
    job_queue->give_one(std::make_shared<Ctrie_Job>(hash_trie, 63));
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
    //const Zeni::Concurrency::Reclamation_Stack::Node * reclamation_stack = nullptr;

    volatile int a = 0;
    for (int i = 0; i != 100000; ++i) {
      //const auto ii = new Int();

      const Zeni::Concurrency::Intrusive_Shared_Ptr<Int>::Lock ii = new Int();

      a += ii->value;

      //delete ii;

      //ii->reclamation_next = reclamation_stack;
      //reclamation_stack = ii;
    }

    if (num_successors)
      get_Job_Queue()->give_one(std::make_shared<Reclamation_Job>(num_successors - 1));

    //while (reclamation_stack) {
    //  const auto ii = reclamation_stack;
    //  reclamation_stack = ii->reclamation_next;
    //  delete ii;
    //}
  }

  const size_t num_successors;
};

void test_Reclamation_Performance(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue)
{
  for (int i = 0; i != 64; ++i)
    job_queue->give_one(std::make_shared<Reclamation_Job>(63));
  worker_threads->finish_jobs();
}

class SmarterRec_Job : public Zeni::Concurrency::Job {
  SmarterRec_Job(const SmarterRec_Job &) = delete;
  SmarterRec_Job & operator =(const SmarterRec_Job &) = delete;
  
public:
  struct Int : public Zeni::Concurrency::Reclamation_Stack::Node, public Zeni::Concurrency::Intrusive_Stack<Int>::Node {
    int value;
  };

  SmarterRec_Job(const std::shared_ptr<Zeni::Concurrency::Intrusive_Stack<Int>> stack_, const size_t num_successors_) : stack(stack_), num_successors(num_successors_) {}

  void execute() noexcept {
    //const Zeni::Concurrency::Reclamation_Stack::Node * reclamation_stack = nullptr;

    volatile int a = 0;
    for (int i = 0; i != 100000; ++i) {
      Int * ii = stack->try_pop();
      if (!ii) {
        ii = new Int();
      }

      a += ii->value;

      stack->push(ii);
    }

    if (num_successors)
      get_Job_Queue()->give_one(std::make_shared<SmarterRec_Job>(stack, num_successors - 1));

    //while (reclamation_stack) {
    //  const auto ii = reclamation_stack;
    //  reclamation_stack = ii->reclamation_next;
    //  delete ii;
    //}
  }

  const std::shared_ptr<Zeni::Concurrency::Intrusive_Stack<Int>> stack;
  const size_t num_successors;
};

void test_SmarterRec_Performance(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue)
{
  const auto stack = std::make_shared<Zeni::Concurrency::Intrusive_Stack<SmarterRec_Job::Int>>();

  for (int i = 0; i != 64; ++i)
    job_queue->give_one(std::make_shared<SmarterRec_Job>(stack, 63));
  worker_threads->finish_jobs();
}
