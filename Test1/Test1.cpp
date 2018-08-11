#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>
#endif

//#include "Zeni/Concurrency/Container/Antiable_Hashset.hpp"
#include "Zeni/Concurrency/Container/Antiable_Hash_Trie.hpp"
#include "Zeni/Concurrency/Container/Antiable_List.hpp"
//#include "Zeni/Concurrency/Container/Ctrie.hpp"
//#include "Zeni/Concurrency/Container/Epoch_List.hpp"
//#include "Zeni/Concurrency/Container/Hash_Trie.hpp"
#include "Zeni/Concurrency/Container/Intrusive_Shared_Ptr.hpp"
#include "Zeni/Concurrency/Container/Ordered_List.hpp"
#include "Zeni/Concurrency/Container/Shared_Ptr.hpp"
#include "Zeni/Concurrency/Container/Stack.hpp"
#include "Zeni/Concurrency/Container/Queue.hpp"
#include "Zeni/Concurrency/Container/Unordered_List.hpp"
#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Concurrency/Message.hpp"
#include "Zeni/Concurrency/Mutex.hpp"
#include "Zeni/Concurrency/Recipient.hpp"
#include "Zeni/Concurrency/Worker_Threads.hpp"

#include "Zeni/Rete/Internal/Debug_Counters.hpp"
#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Node_Filter.hpp"
#include "Zeni/Rete/Node_Passthrough_Gated.hpp"
#include "Zeni/Rete/Node_Unary_Gate.hpp"
#include "Zeni/Rete/Parser.hpp"

#include <array>
#include <atomic>
#include <iostream>
#include <list>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>

class Whisper : public Zeni::Concurrency::Message {
public:
  typedef std::shared_ptr<Whisper> Ptr;

  Whisper(const std::shared_ptr<Zeni::Concurrency::Recipient> &recipient, const std::string_view message)
    : Message(recipient), m_message(message)
  {
  }

  std::string get_message() const {
    return m_message;
  }

private:
  std::string m_message;
};

//static ZENI_CONCURRENCY_CACHE_ALIGN std::atomic_int64_t g_num_recvs = 0;

class Gossip : public Zeni::Concurrency::Recipient {
public:
  typedef std::shared_ptr<Gossip> Ptr;

  void receive(const std::shared_ptr<const Zeni::Concurrency::Message> message) noexcept override {
    const auto whisper = std::dynamic_pointer_cast<const Whisper>(message);

    std::cerr << whisper->get_message() + "\n";
    //++g_num_recvs;

    std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
    Zeni::Concurrency::Mutex::Lock mutex_lock(m_mutex);
    for (Ptr gossip : m_gossips)
      jobs.emplace_back(std::make_shared<Whisper>(gossip, whisper->get_message()));

    const auto job_queue = message->get_Job_Queue();
    job_queue->give_many(std::move(jobs));
  }

  void tell(const Ptr gossip) {
    Zeni::Concurrency::Mutex::Lock mutex_lock(m_mutex);
    m_gossips.emplace(gossip);
  }

private:
  std::unordered_set<Ptr> m_gossips;
};

static void test_Worker_Threads(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_Stack(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_Queue(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_Queue_of_Shared_Ptrs(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_Queue_of_Intrusive_Shared_Ptrs(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
//static void test_Epoch_List(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_Unordered_List(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_Ordered_List(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
//static void test_Epochs_in_Stack(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_Antiable_List(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
//static void test_Antiable_Hashset(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
//static void test_Ctrie(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
//static void test_Hash_Trie(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_Antiable_Hash_Trie(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_Rete_Network(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_Parser(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);

//struct ZENI_CONCURRENCY_CACHE_ALIGN_TOGETHER UInt128 {
//  UInt128() = default;
//  UInt128(const uint64_t first_, const uint64_t second_) : second(second_), first(first_) {}
//
//  uint64_t second = 0;
//  uint64_t first = 0;
//};
//
//#ifdef _MSC_VER
//#include <Windows.h>
//#undef min
//#undef max
//#endif
//
//struct ZENI_CONCURRENCY_CACHE_ALIGN_TOGETHER Atomic_UInt128 {
//  Atomic_UInt128() = default;
//  Atomic_UInt128(const uint64_t first_, const uint64_t second_) : second(second_), first(first_) {}
//  Atomic_UInt128(const UInt128 value) : second(value.second), first(value.first) {}
//
//  inline bool compare_exchange_strong(UInt128 &expected, const UInt128 desired) {
//#ifdef _MSC_VER
//    return InterlockedCompareExchange128(reinterpret_cast<volatile LONG64 *>(this),
//      reinterpret_cast<const LONG64 &>(desired.first),
//      reinterpret_cast<const LONG64 &>(desired.second),
//      reinterpret_cast<LONG64 *>(&expected));
//#else
//    bool result;
//    __asm__ __volatile__
//    (
//      "lock cmpxchg16b %1\n\t"
//      "setz %0"
//      : "=q" (result)
//      , "+m" (*this)
//      , "+d" (expected.first)
//      , "+a" (expected.second)
//      : "c" (desired.first)
//      , "b" (desired.second)
//      : "cc"
//    );
//    return result;
//#endif
//  }
//
//  volatile uint64_t second = 0;
//  volatile uint64_t first = 0;
//};

int main()
{
#ifdef _MSC_VER
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  //_CrtSetBreakAlloc(42);
#endif

  auto worker_threads = Zeni::Concurrency::Worker_Threads::Create();
  const auto job_queue = worker_threads->get_main_Job_Queue();

  //Atomic_UInt128 aa(1, 2);
  //UInt128 a(1, 2), b(3, 4), c(5, 6);
  //std::cout << aa.first << aa.second << b.first << b.second;
  //aa.compare_exchange_strong(b, c);
  //std::cout << aa.first << aa.second << b.first << b.second;
  //aa.compare_exchange_strong(b, c);
  //std::cout << aa.first << aa.second << b.first << b.second;

//  test_Worker_Threads(worker_threads, job_queue);
//  if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
//    std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
//    abort();
//  }
//
//#if ZENI_CONCURRENCY != ZENI_CONCURRENCY_NONE
//  for (int i = 0; i != 80; ++i) {
//    test_Queue(worker_threads, job_queue);
//    if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
//      std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
//      abort();
//    }
//    std::cout << 'Q' << std::flush;
//  }
//  std::cout << std::endl;

  //for (int i = 0; i != 80000; ++i) {
  //  test_Queue_of_Shared_Ptrs(worker_threads, job_queue);
  //  //if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
  //  //  std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
  //  //  abort();
  //  //}
  //  std::cout << 'P' << std::flush;
  //}
  //std::cout << std::endl;

  //for (int i = 0; i != 80; ++i) {
  //  test_Queue_of_Intrusive_Shared_Ptrs(worker_threads, job_queue);
  //  //if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
  //  //  std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
  //  //  abort();
  //  //}
  //  std::cout << 'I' << std::flush;
  //}
  //std::cout << std::endl;

//  for (int i = 0; i != 80; ++i) {
//    test_Stack(worker_threads, job_queue);
//    if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
//      std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
//      abort();
//    }
//    std::cout << 'S' << std::flush;
//  }
//  std::cout << std::endl;
//
//  for (int i = 0; i != 80; ++i) {
//    test_Ordered_List(worker_threads, job_queue);
//    if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
//      std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
//      abort();
//    }
//    std::cout << 'O' << std::flush;
//  }
//  std::cout << std::endl;
//
//  for (int i = 0; i != 80; ++i) {
//    test_Unordered_List(worker_threads, job_queue);
//    if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
//      std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
//      abort();
//    }
//    std::cout << 'U' << std::flush;
//  }
//  std::cout << std::endl;
//#endif

  //for (int i = 0; i != 80; ++i) {
  //  test_Epoch_List(worker_threads, job_queue);
  //  //if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
  //  //  std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
  //  //  abort();
  //  //}
  //  std::cout << 'e' << std::flush;
  //}
  //std::cout << std::endl;

  //for (int i = 0; i != 80000; ++i) {
  //  test_Epochs_in_Stack(worker_threads, job_queue);
  //  //if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
  //  //  std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
  //  //  abort();
  //  //}
  //  std::cout << 'E' << std::flush;
  //}
  //std::cout << std::endl;

  //for (int i = 0; i != 80000; ++i) {
  //  test_Antiable_List(worker_threads, job_queue);
  //  //if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
  //  //  std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
  //  //  abort();
  //  //}
  //  std::cout << 'A' << std::flush;
  //}
  //std::cout << std::endl;

  //for (int i = 0; i != 80000; ++i) {
  //  test_Antiable_Hashset(worker_threads, job_queue);
  //  //if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
  //  //  std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
  //  //  abort();
  //  //}
  //  std::cout << 'h' << std::flush;
  //}
  //std::cout << std::endl;

  //for (int i = 0; i != 80000; ++i) {
  //  test_Ctrie(worker_threads, job_queue);
  //  //if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
  //  //  std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
  //  //  abort();
  //  //}
  //  std::cout << 'C' << std::flush;
  //}
  //std::cout << std::endl;

  for (int i = 0; i != 80000; ++i) {
    test_Antiable_Hash_Trie(worker_threads, job_queue);
    //if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
    //  std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
    //  abort();
    //}
    std::cout << 'H' << std::flush;
  }
  std::cout << std::endl;

  Zeni::Rete::Debug_Counters::print(std::cerr);
  test_Rete_Network(worker_threads, job_queue);
  Zeni::Rete::Debug_Counters::print(std::cerr);
  //if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
  //  std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
  //  abort();
  //}
  Zeni::Rete::Debug_Counters::reset();

  std::cerr << "Test: ";
  for (int i = 1; i != 11; ++i) {
    std::cerr << ' ' << i;
    test_Rete_Network(worker_threads, job_queue);
    //if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
    //  std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
    //  Zeni::Rete::Debug_Counters::print(std::cerr);
    //  abort();
    //}
    Zeni::Rete::Debug_Counters::reset();
  }

  std::cerr << std::endl;

  test_Parser(worker_threads, job_queue);
  //if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
  //  std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
  //  Zeni::Rete::Debug_Counters::print(std::cerr);
  //  abort();
  //}

  return 0;
}

void test_Worker_Threads(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue) {
  std::unordered_map<std::string, Gossip::Ptr> gossips;

  for (std::string name : {"alice", "betty", "carol", "diane", "ellen", "farah", "gabby", "helen", "irene", "janae", "kelly"})
    gossips[name] = std::make_shared<Gossip>();

  gossips["alice"]->tell(gossips["betty"]);
  gossips["betty"]->tell(gossips["carol"]);
  gossips["betty"]->tell(gossips["diane"]);
  gossips["carol"]->tell(gossips["ellen"]);
  gossips["carol"]->tell(gossips["farah"]);
  gossips["carol"]->tell(gossips["gabby"]);
  gossips["diane"]->tell(gossips["helen"]);
  gossips["diane"]->tell(gossips["irene"]);
  gossips["diane"]->tell(gossips["janae"]);
  gossips["diane"]->tell(gossips["kelly"]);

  std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
  for (std::string message : {"Hi.", "Want to hear a joke?", "Why did the chicken cross the road?", "To get to the other side!"})
    jobs.emplace_back(std::make_shared<Whisper>(gossips["alice"], message));

  job_queue->give_many(std::move(jobs));
  jobs.clear();

  //for(int i = 0; i != 1000000; ++i)
  //  worker_threads.get_queue()->give(std::make_shared<Whisper>(gossips["alice"], "Meh."));

  worker_threads->finish_jobs();

  for (std::string message : {"I get it.", "That was a bad one.", "I'm sorry. :-("})
    jobs.emplace_back(std::make_shared<Whisper>(gossips["alice"], message));
  job_queue->give_many(std::move(jobs));

  //std::cout << "g_num_recvs == " << g_num_recvs << std::endl;
}

void test_Queue(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue) {
  class Pusher : public Zeni::Concurrency::Job {
  public:
    Pusher(const std::shared_ptr<Zeni::Concurrency::Queue<int>> &queue) : m_queue(queue) {}

    void execute() noexcept override {
      for (int i = 0; i != 10000; ++i)
        m_queue->push(i);
      while (!m_queue->empty());
    }

  private:
    std::shared_ptr<Zeni::Concurrency::Queue<int>> m_queue;
  };

  class Popper : public Zeni::Concurrency::Job {
  public:
    Popper(const std::shared_ptr<Zeni::Concurrency::Queue<int>> &queue) : m_queue(queue) {}

    void execute() noexcept override {
      for (int i = 0; i != 10000; ) {
        int value;
        if (m_queue->try_pop(value)) {
          ++i;
          //std::cout << value;
        }
      }
      while (!m_queue->empty());
    }

  private:
    std::shared_ptr<Zeni::Concurrency::Queue<int>> m_queue;
  };

  const auto queue = std::make_shared<Zeni::Concurrency::Queue<int>>();

  std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
  for (uint64_t i = 0; i != std::thread::hardware_concurrency() / 2; ++i) {
    jobs.emplace_back(std::make_shared<Pusher>(queue));
    jobs.emplace_back(std::make_shared<Popper>(queue));
  }
  job_queue->give_many(std::move(jobs));

  worker_threads->finish_jobs();

  //std::cout << std::endl;
}

void test_Queue_of_Shared_Ptrs(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue) {
  class Pusher : public Zeni::Concurrency::Job {
  public:
    Pusher(const std::shared_ptr<Zeni::Concurrency::Queue<Zeni::Concurrency::Shared_Ptr<int>>> &queue) : m_queue(queue) {}

    void execute() noexcept override {
      for (int i = 0; i != 10000; ++i) {
        Zeni::Concurrency::Shared_Ptr<int> i_ptr(new int(i));
        m_queue->push(i_ptr);
        i_ptr.reset();
      }
      while (!m_queue->empty());
    }

  private:
    std::shared_ptr<Zeni::Concurrency::Queue<Zeni::Concurrency::Shared_Ptr<int>>> m_queue;
  };

  class Popper : public Zeni::Concurrency::Job {
  public:
    Popper(const std::shared_ptr<Zeni::Concurrency::Queue<Zeni::Concurrency::Shared_Ptr<int>>> &queue) : m_queue(queue) {}

    void execute() noexcept override {
      Zeni::Concurrency::Shared_Ptr<int> i_ptr;
      for (int i = 0; i != 10000; ) {
        if (m_queue->try_pop(i_ptr)) {
          ++i;
          //std::cout << value;
        }
      }
      while (!m_queue->empty());
    }

  private:
    std::shared_ptr<Zeni::Concurrency::Queue<Zeni::Concurrency::Shared_Ptr<int>>> m_queue;
  };

  const auto queue = std::make_shared<Zeni::Concurrency::Queue<Zeni::Concurrency::Shared_Ptr<int>>>();

  std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
  for (uint64_t i = 0; i != std::thread::hardware_concurrency() / 2; ++i) {
    jobs.emplace_back(std::make_shared<Pusher>(queue));
    jobs.emplace_back(std::make_shared<Popper>(queue));
  }
  job_queue->give_many(std::move(jobs));

  worker_threads->finish_jobs();

  //std::cout << std::endl;
}

void test_Queue_of_Intrusive_Shared_Ptrs(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue) {
  struct Intruded_Int : public Zeni::Concurrency::Enable_Intrusive_Sharing<Intruded_Int> {
    Intruded_Int() = default;
    Intruded_Int(const int value_) : value(value_) {}

    int value = 0;
  };

  class Pusher : public Zeni::Concurrency::Job {
  public:
    Pusher(const std::shared_ptr<Zeni::Concurrency::Queue<Zeni::Concurrency::Intrusive_Shared_Ptr<Intruded_Int>>> &queue) : m_queue(queue) {}

    void execute() noexcept override {
      for (int i = 0; i != 10000; ++i) {
        Zeni::Concurrency::Shared_Ptr<int> i_ptr(new int(i));
        m_queue->push(Zeni::Concurrency::Intrusive_Shared_Ptr<Intruded_Int>(new Intruded_Int(i)));
        i_ptr.reset();
      }
      while (!m_queue->empty());
    }

  private:
    std::shared_ptr<Zeni::Concurrency::Queue<Zeni::Concurrency::Intrusive_Shared_Ptr<Intruded_Int>>> m_queue;
  };

  class Popper : public Zeni::Concurrency::Job {
  public:
    Popper(const std::shared_ptr<Zeni::Concurrency::Queue<Zeni::Concurrency::Intrusive_Shared_Ptr<Intruded_Int>>> &queue) : m_queue(queue) {}

    void execute() noexcept override {
      Zeni::Concurrency::Intrusive_Shared_Ptr<Intruded_Int> i_ptr;
      for (int i = 0; i != 10000; ) {
        if (m_queue->try_pop(i_ptr)) {
          ++i;
          //std::cout << value;
        }
      }
      while (!m_queue->empty());
    }

  private:
    std::shared_ptr<Zeni::Concurrency::Queue<Zeni::Concurrency::Intrusive_Shared_Ptr<Intruded_Int>>> m_queue;
  };

  const auto queue = std::make_shared<Zeni::Concurrency::Queue<Zeni::Concurrency::Intrusive_Shared_Ptr<Intruded_Int>>>();

  std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
  for (uint64_t i = 0; i != std::thread::hardware_concurrency() / 2; ++i) {
    jobs.emplace_back(std::make_shared<Pusher>(queue));
    jobs.emplace_back(std::make_shared<Popper>(queue));
  }
  job_queue->give_many(std::move(jobs));

  worker_threads->finish_jobs();

  //std::cout << std::endl;
}

void test_Stack(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue) {
  class Pusher : public Zeni::Concurrency::Job {
  public:
    Pusher(const std::shared_ptr<Zeni::Concurrency::Stack<int>> &stack) : m_stack(stack) {}

    void execute() noexcept override {
      for (int i = 0; i != 10000; ++i)
        m_stack->push(i);
      while (!m_stack->empty());
    }

  private:
    std::shared_ptr<Zeni::Concurrency::Stack<int>> m_stack;
  };

  class Popper : public Zeni::Concurrency::Job {
  public:
    Popper(const std::shared_ptr<Zeni::Concurrency::Stack<int>> &stack) : m_stack(stack) {}

    void execute() noexcept override {
      for (int i = 0; i != 10000; ) {
        int value;
        if (m_stack->try_pop(value)) {
          ++i;
          //std::cout << value;
        }
      }
      while (!m_stack->empty());
    }

  private:
    std::shared_ptr<Zeni::Concurrency::Stack<int>> m_stack;
  };

  const auto stack = std::make_shared<Zeni::Concurrency::Stack<int>>();

  std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
  for (uint64_t i = 0; i != std::thread::hardware_concurrency() / 2; ++i) {
    jobs.emplace_back(std::make_shared<Pusher>(stack));
    jobs.emplace_back(std::make_shared<Popper>(stack));
  }
  job_queue->give_many(std::move(jobs));

  worker_threads->finish_jobs();

  //std::cout << std::endl;
}

//void test_Epoch_List(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue) {
//  class Epocher : public Zeni::Concurrency::Job {
//  public:
//    Epocher(const std::shared_ptr<Zeni::Concurrency::Epoch_List> &epoch_list) : m_epoch_list(epoch_list), dre(rd()) {}
//
//    void execute() noexcept override {
//      while (m_to_acquire + m_to_release != 0) {
//        const int64_t index = std::uniform_int_distribution<int64_t>(1, std::min(m_to_acquire, m_acquire_cap - m_to_release) + m_to_release)(dre);
//        if (index > m_to_release) {
//          const auto epoch = Zeni::Concurrency::Epoch_List::Create_Token();
//          if (std::uniform_int_distribution<int>(0, 1)(dre)) {
//            m_epoch_list->acquire_release(epoch);
//            --m_to_acquire;
//          }
//          else
//          {
//            m_epoch_list->acquire(epoch);
//            m_epochs.push_back(epoch.load());
//            --m_to_acquire;
//            ++m_to_release;
//          }
//        }
//        else {
//          auto selected = m_epochs.begin();
//          std::advance(selected, index - 1);
//          [[maybe_unused]] const bool success = m_epoch_list->try_release(*selected);
//          if (!success)
//            std::cerr << 'X' << std::flush;
//          m_epochs.erase(selected);
//          --m_to_release;
//        }
//      }
//    }
//
//  private:
//    std::shared_ptr<Zeni::Concurrency::Epoch_List> m_epoch_list;
//    std::vector<Zeni::Concurrency::Epoch_List::Token_Ptr::Lock> m_epochs;
//    int64_t m_to_acquire = 1024;
//    int64_t m_acquire_cap = 1;
//    int64_t m_to_release = 0;
//    std::random_device rd;
//    std::default_random_engine dre;
//  };
//
//  const auto epoch_list = std::make_shared<Zeni::Concurrency::Epoch_List>();
//
//  std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
//  for (uint64_t i = 0; i != std::thread::hardware_concurrency(); ++i)
//    jobs.emplace_back(std::make_shared<Epocher>(epoch_list));
//  job_queue->give_many(std::move(jobs));
//
//  worker_threads->finish_jobs();
//
//  //std::cout << std::endl;
//}

void test_Unordered_List(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue) {
  class Lister : public Zeni::Concurrency::Job {
  public:
    Lister(const std::shared_ptr<Zeni::Concurrency::Unordered_List<int64_t>> &unordered_list) : m_unordered_list(unordered_list), dre(rd()) {}

    void execute() noexcept override {
      while (m_to_acquire + m_to_release != 0) {
        const int64_t index = std::uniform_int_distribution<int64_t>(1, std::min(m_to_acquire, m_acquire_cap - m_to_release) + m_to_release)(dre);
        if (index > m_to_release) {
          if (m_to_release & 1)
            m_unordered_list->push_front(m_to_acquire);
          else
            m_unordered_list->push_back(m_to_acquire);
          m_values.push_back(m_to_acquire);
          --m_to_acquire;
          ++m_to_release;
        }
        else {
          auto selected = m_values.begin();
          std::advance(selected, index - 1);
          [[maybe_unused]] const bool success = m_unordered_list->try_erase(*selected);
          if (!success)
            std::cerr << 'X' << std::flush;
          m_values.erase(selected);
          --m_to_release;
        }
      }
    }

  private:
    std::shared_ptr<Zeni::Concurrency::Unordered_List<int64_t>> m_unordered_list;
    std::vector<uint64_t> m_values;
    int64_t m_to_acquire = 256;
    int64_t m_acquire_cap = 16;
    int64_t m_to_release = 0;
    std::random_device rd;
    std::default_random_engine dre;
  };

  const auto unordered_list = std::make_shared<Zeni::Concurrency::Unordered_List<int64_t>>();

  std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
  for (uint64_t i = 0; i != std::thread::hardware_concurrency(); ++i)
    jobs.emplace_back(std::make_shared<Lister>(unordered_list));
  job_queue->give_many(std::move(jobs));

  worker_threads->finish_jobs();

  //std::cout << std::endl;
}

void test_Ordered_List(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue) {
  class Lister : public Zeni::Concurrency::Job {
  public:
    Lister(const std::shared_ptr<Zeni::Concurrency::Ordered_List<uint64_t>> &ordered_list) : m_ordered_list(ordered_list), dre(rd()) {}

    void execute() noexcept override {
      m_values_to_acquire.reserve(m_to_acquire);
      for (uint64_t i = 0; i != m_to_acquire; ++i)
        m_values_to_acquire.push_back(i);

      while (m_to_acquire + m_to_release != 0) {
        uint64_t front;
        m_ordered_list->front(front);

        const uint64_t index = std::uniform_int_distribution<uint64_t>(1, std::min(m_to_acquire, m_acquire_cap - m_to_release) + m_to_release)(dre);
        if (index > m_to_release) {
          const uint64_t index2 = std::uniform_int_distribution<uint64_t>(0, m_to_acquire - 1)(dre);
          auto selected = m_values_to_acquire.begin();
          std::advance(selected, index2);
          m_ordered_list->insert(*selected);
          m_values_to_release.push_back(*selected);
          m_values_to_acquire.erase(selected);
          --m_to_acquire;
          ++m_to_release;
        }
        else {
          auto selected = m_values_to_release.begin();
          std::advance(selected, index - 1);
          [[maybe_unused]] const bool success = m_ordered_list->try_erase(*selected);
          if (!success)
            std::cerr << 'X' << std::flush;
          m_values_to_release.erase(selected);
          --m_to_release;
        }
      }
    }

  private:
    std::shared_ptr<Zeni::Concurrency::Ordered_List<uint64_t>> m_ordered_list;
    std::vector<uint64_t> m_values_to_acquire;
    std::vector<uint64_t> m_values_to_release;
    uint64_t m_to_acquire = 256;
    uint64_t m_acquire_cap = 16;
    uint64_t m_to_release = 0;
    std::random_device rd;
    std::default_random_engine dre;
  };

  const auto ordered_list = std::make_shared<Zeni::Concurrency::Ordered_List<uint64_t>>();

  std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
  for (uint64_t i = 0; i != std::thread::hardware_concurrency(); ++i)
    jobs.emplace_back(std::make_shared<Lister>(ordered_list));
  job_queue->give_many(std::move(jobs));

  worker_threads->finish_jobs();

  //std::cout << std::endl;
}

//void test_Epochs_in_Stack(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue) {
//  class Antiable : public Zeni::Concurrency::Job {
//  public:
//    Antiable(const std::shared_ptr<Zeni::Concurrency::Epoch_List> &epoch_list,
//      const std::shared_ptr<Zeni::Concurrency::Stack<Zeni::Concurrency::Epoch_List::Token_Ptr>> &token_stack,
//      const uint64_t to_acquire,
//      std::atomic_bool &failed)
//      : m_epoch_list(epoch_list),
//      m_token_stack(token_stack),
//      m_to_acquire(to_acquire),
//      m_failed(failed),
//      dre(rd())
//    {
//    }
//
//    void execute() noexcept override {
//      while (m_to_acquire != 0) {
//        Zeni::Concurrency::Epoch_List::Token_Ptr::Lock insertion_epoch;
//        auto token = Zeni::Concurrency::Epoch_List::Create_Token();
//        m_token_stack->push(token);
//        m_epoch_list->acquire(token);
//        Zeni::Concurrency::Stack<Zeni::Concurrency::Epoch_List::Token_Ptr>::Node * head = m_token_stack->m_head;
//        std::multiset<uint64_t> epochs_captured;
//        for (auto node = head; node; node = node->next) {
//          const uint64_t epoch = node->value.load()->epoch();
//          if (epoch == 0)
//            continue;
//          epochs_captured.insert(epoch);
//        }
//        auto it = epochs_captured.cbegin();
//        for (uint64_t i = 1, iend = token.load()->epoch() + Zeni::Concurrency::Epoch_List::epoch_increment; i != iend; ++it,  i += Zeni::Concurrency::Epoch_List::epoch_increment) {
//          if (it == epochs_captured.cend() || *it != i)
//            abort();
//        }
//        --m_to_acquire;
//      }
//    }
//
//  private:
//    std::shared_ptr<Zeni::Concurrency::Epoch_List> m_epoch_list;
//    std::shared_ptr<Zeni::Concurrency::Stack<Zeni::Concurrency::Epoch_List::Token_Ptr>> m_token_stack;
//    uint64_t m_to_acquire;
//    std::atomic_bool &m_failed;
//    std::random_device rd;
//    std::default_random_engine dre;
//  };
//
//  const auto epoch_list = std::make_shared<Zeni::Concurrency::Epoch_List>();
//  const auto token_stack = std::make_shared<Zeni::Concurrency::Stack<Zeni::Concurrency::Epoch_List::Token_Ptr>>();
//  ZENI_CONCURRENCY_CACHE_ALIGN std::atomic_bool failed = false;
//
//  std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
//  for (uint64_t i = 0; i != std::thread::hardware_concurrency(); ++i)
//    jobs.emplace_back(std::make_shared<Antiable>(epoch_list, token_stack, 8, failed));
//  job_queue->give_many(std::move(jobs));
//
//  worker_threads->finish_jobs();
//
//  if (failed.load()) {
//    std::cerr << 'Y';
//    abort();
//  }
//
//  //std::cout << std::endl;
//}

//#define DEBUG_HARD

void test_Antiable_List(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue) {
  class Antiable : public Zeni::Concurrency::Job {
  public:
    Antiable(const std::shared_ptr<Zeni::Concurrency::Epoch_List> &epoch_list,
      const std::shared_ptr<Zeni::Concurrency::Antiable_List<int64_t>> &antiable_list1,
      const std::shared_ptr<Zeni::Concurrency::Antiable_List<int64_t>> &antiable_list2,
      const uint64_t to_acquire,
#ifdef DEBUG_HARD
      const std::shared_ptr<Zeni::Concurrency::Queue<std::string>> &debug_output,
#endif
      std::atomic_int64_t &sum)
      : m_epoch_list(epoch_list),
      m_antiable_list1(antiable_list1),
      m_antiable_list2(antiable_list2),
      m_to_acquire(to_acquire),
#ifdef DEBUG_HARD
      m_debug_output(debug_output),
#endif
      m_sum(sum),
      dre(rd())
    {
    }

    void execute() noexcept override {
      m_values_to_acquire.reserve(m_to_acquire);
      m_values_to_release.reserve(m_to_acquire);
      for (uint64_t i = 1; i != m_to_acquire + 1; ++i) {
        m_values_to_acquire.push_back(int64_t(i));
        m_values_to_release.push_back(int64_t(i));
      }
      while (!m_values_to_acquire.empty() || !m_values_to_release.empty()) {
        const size_t index = std::uniform_int_distribution<size_t>(0, m_values_to_acquire.size() + m_values_to_release.size() - 1)(dre);
        if (index < m_values_to_acquire.size()) {
          auto selected = m_values_to_acquire.begin();
          std::advance(selected, index);
          Zeni::Concurrency::Epoch_List::Token_Ptr::Lock insertion_epoch;
#ifdef DEBUG_HARD
          std::ostringstream oss;
          oss << std::this_thread::get_id() << " +" << *selected;
#endif
          if (m_antiable_list1->insert(m_epoch_list, *selected, insertion_epoch)) {
#ifdef DEBUG_HARD
            oss << " @" << insertion_epoch->epoch() << " *:";
#endif
            for (auto it = m_antiable_list2->cbegin(insertion_epoch), iend = m_antiable_list2->cend(); it != iend; ++it) {
              m_sum += *selected * *it;
#ifdef DEBUG_HARD
              oss << ' ' << *it << "@[" << it.creation_epoch() << ',' << it.deletion_epoch() << ')';
#endif
            }
            if (insertion_epoch)
              m_epoch_list->try_release(insertion_epoch);
            else
              std::cerr << 'x';
          }
#ifdef DEBUG_HARD
          m_debug_output->push(oss.str());
#endif
          m_values_to_acquire.erase(selected);
        }
        else {
          auto selected = m_values_to_release.begin();
          std::advance(selected, index - m_values_to_acquire.size());
          Zeni::Concurrency::Epoch_List::Token_Ptr::Lock erasure_epoch;
#ifdef DEBUG_HARD
          std::ostringstream oss;
          oss << std::this_thread::get_id() << " -" << *selected;
#endif
          if (m_antiable_list1->erase(m_epoch_list, *selected, erasure_epoch)) {
#ifdef DEBUG_HARD
            oss << " @" << erasure_epoch->epoch() << " *:";
#endif
            for (auto it = m_antiable_list2->cbegin(erasure_epoch), iend = m_antiable_list2->cend(); it != iend; ++it) {
              m_sum -= *selected * *it;
#ifdef DEBUG_HARD
              oss << ' ' << *it << "@[" << it.creation_epoch() << ',' << it.deletion_epoch() << ')';
#endif
            }
            if (erasure_epoch)
              m_epoch_list->try_release(erasure_epoch);
            else
              std::cerr << 'y';
          }
#ifdef DEBUG_HARD
          m_debug_output->push(oss.str());
#endif
          m_values_to_release.erase(selected);
        }
      }
    }

  private:
    std::shared_ptr<Zeni::Concurrency::Epoch_List> m_epoch_list;
    std::shared_ptr<Zeni::Concurrency::Antiable_List<int64_t>> m_antiable_list1;
    std::shared_ptr<Zeni::Concurrency::Antiable_List<int64_t>> m_antiable_list2;
    uint64_t m_to_acquire;
#ifdef DEBUG_HARD
    std::shared_ptr<Zeni::Concurrency::Queue<std::string>> m_debug_output;
#endif
    ZENI_CONCURRENCY_CACHE_ALIGN std::atomic_int64_t &m_sum;
    std::vector<uint64_t> m_values_to_acquire;
    std::vector<uint64_t> m_values_to_release;
    std::random_device rd;
    std::default_random_engine dre;
  };

  const auto epoch_list = std::make_shared<Zeni::Concurrency::Epoch_List>();
  const auto antiable_list1 = std::make_shared<Zeni::Concurrency::Antiable_List<int64_t>>();
  const auto antiable_list2 = std::make_shared<Zeni::Concurrency::Antiable_List<int64_t>>();
#ifdef DEBUG_HARD
  const auto debug_output = std::make_shared<Zeni::Concurrency::Queue<std::string>>();
#endif
  ZENI_CONCURRENCY_CACHE_ALIGN std::atomic_int64_t sum = 0;

  std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
  for (uint64_t i = 0; i != std::thread::hardware_concurrency() / 2; ++i) {
    jobs.emplace_back(std::make_shared<Antiable>(epoch_list, antiable_list1, antiable_list1, 4,
#ifdef DEBUG_HARD
      debug_output,
#endif
      sum));
    jobs.emplace_back(std::make_shared<Antiable>(epoch_list, antiable_list1, antiable_list1, 4,
#ifdef DEBUG_HARD
      debug_output,
#endif
      sum));
  }
  job_queue->give_many(std::move(jobs));

  worker_threads->finish_jobs();

  if (antiable_list1->size() != 0 || antiable_list1->usage() != 0 || antiable_list2->size() != 0 || antiable_list2->usage() != 0)
    std::cerr << 'X';
  if (sum.load(std::memory_order_relaxed) != 0) {
#ifdef DEBUG_HARD
    std::cerr << std::endl;
    std::string line;
    while (debug_output->try_pop(line))
    std::cerr << line << std::endl;
    std::cerr << sum << std::endl;
#else
    std::cerr << 'Y';
#endif
    abort();
  }

  //std::cout << std::endl;
}

//#define DEBUG_HARD
//
//void test_Antiable_Hashset(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue) {
//  class Antiable : public Zeni::Concurrency::Job {
//  public:
//    Antiable(const std::shared_ptr<Zeni::Concurrency::Epoch_List> &epoch_list,
//      const std::shared_ptr<Zeni::Concurrency::Antiable_Hashset<int64_t>> &antiable_hashset1,
//      const std::shared_ptr<Zeni::Concurrency::Antiable_Hashset<int64_t>> &antiable_hashset2,
//      const uint64_t to_acquire,
//#ifdef DEBUG_HARD
//      const std::shared_ptr<Zeni::Concurrency::Queue<std::string>> &debug_output,
//#endif
//      std::atomic_int64_t &sum)
//      : m_epoch_list(epoch_list),
//      m_antiable_hashset1(antiable_hashset1),
//      m_antiable_hashset2(antiable_hashset2),
//      m_to_acquire(to_acquire),
//#ifdef DEBUG_HARD
//      m_debug_output(debug_output),
//#endif
//      m_sum(sum),
//      dre(rd())
//    {
//    }
//
//    void execute() noexcept override {
//      m_values_to_acquire.reserve(m_to_acquire);
//      m_values_to_release.reserve(m_to_acquire);
//      for (uint64_t i = 1; i != m_to_acquire + 1; ++i) {
//        m_values_to_acquire.push_back(int64_t(i));
//        m_values_to_release.push_back(int64_t(i));
//      }
//      while (!m_values_to_acquire.empty() || !m_values_to_release.empty()) {
//        const size_t index = std::uniform_int_distribution<size_t>(0, m_values_to_acquire.size() + m_values_to_release.size() - 1)(dre);
//        if (index < m_values_to_acquire.size()) {
//          auto selected = m_values_to_acquire.begin();
//          std::advance(selected, index);
//          Zeni::Concurrency::Epoch_List::Token_Ptr::Lock insertion_epoch;
//#ifdef DEBUG_HARD
//          std::ostringstream oss, oss2;
//          oss << std::this_thread::get_id() << " +" << *selected;
//#endif
//          if (m_antiable_hashset1->insert(m_epoch_list, *selected, insertion_epoch)) {
//#ifdef DEBUG_HARD
//            oss << " @" << insertion_epoch->epoch() << " *:";
//            oss2 << " @" << insertion_epoch->epoch() << " *:";
//#endif
//            int64_t sum0 = 0;
//            Zeni::Concurrency::Antiable_Hashset<int64_t>::Node * head = m_antiable_hashset2->m_head;
//            const auto it0 = m_antiable_hashset2->cbegin(insertion_epoch);
//            for (auto it = it0, iend = m_antiable_hashset2->cend(); it != iend; ++it) {
//              sum0 += *selected * *it;
//#ifdef DEBUG_HARD
//              oss << ' ' << *it << "@[" << it.creation_epoch() << ',' << it.deletion_epoch() << ')';
//#endif
//            }
//#ifdef DEBUG_HARD
//            int64_t sum1 = 0;
//            const auto it1 = m_antiable_hashset2->cbegin(insertion_epoch);
//            if (it0 != it1) {
//              std::cerr << "\nDifferent heads" << std::endl;
//              auto cur = head;
//              while (cur && cur != it1.m_node)
//                cur = reinterpret_cast<Zeni::Concurrency::Antiable_Hashset<int64_t>::Node *>(uintptr_t(cur->next_in_list.load()) & ~uintptr_t(0x1));
//              std::cerr << (cur ? "Missing head found!" : "Missing head NOT found :-(") << std::endl;
//            }
//            for (auto it = it1, iend = m_antiable_hashset2->cend(); it != iend; ++it) {
//              sum1 += *selected * *it;
//              oss2 << ' ' << *it << "@[" << it.creation_epoch() << ',' << it.deletion_epoch() << ')';
//            }
//            if (sum0 != sum1) {
//              m_debug_output->push(oss.str() + " :: " + oss2.str());
//              break;
//            }
//#endif
//            m_sum.fetch_add(sum0, std::memory_order_relaxed);
//            if (insertion_epoch)
//              m_epoch_list->try_release(insertion_epoch);
//            else
//              std::cerr << 'x';
//          }
//#ifdef DEBUG_HARD
//          m_debug_output->push(oss.str());
//#endif
//          m_values_to_acquire.erase(selected);
//        }
//        else {
//          auto selected = m_values_to_release.begin();
//          std::advance(selected, index - m_values_to_acquire.size());
//          Zeni::Concurrency::Epoch_List::Token_Ptr::Lock erasure_epoch;
//#ifdef DEBUG_HARD
//          std::ostringstream oss, oss2;
//          oss << std::this_thread::get_id() << " -" << *selected;
//#endif
//          if (m_antiable_hashset1->erase(m_epoch_list, *selected, erasure_epoch)) {
//#ifdef DEBUG_HARD
//            oss << " @" << erasure_epoch->epoch() << " *:";
//            oss2 << " @" << erasure_epoch->epoch() << " *:";
//#endif
//            int64_t sum0 = 0;
//            const auto it0 = m_antiable_hashset2->cbegin(erasure_epoch);
//            Zeni::Concurrency::Antiable_Hashset<int64_t>::Node * head = m_antiable_hashset2->m_head;
//            for (auto it = it0, iend = m_antiable_hashset2->cend(); it != iend; ++it) {
//              sum0 += *selected * *it;
//#ifdef DEBUG_HARD
//              oss << ' ' << *it << "@[" << it.creation_epoch() << ',' << it.deletion_epoch() << ')';
//#endif
//            }
//#ifdef DEBUG_HARD
//            int64_t sum1 = 0;
//            const auto it1 = m_antiable_hashset2->cbegin(erasure_epoch);
//            if (it0 != it1) {
//              std::cerr << "\nDifferent heads" << std::endl;
//              auto cur = head;
//              while (cur && cur != it1.m_node)
//                cur = reinterpret_cast<Zeni::Concurrency::Antiable_Hashset<int64_t>::Node *>(uintptr_t(cur->next_in_list.load()) & ~uintptr_t(0x1));
//              std::cerr << (cur ? "Missing head found!" : "Missing head NOT found :-(") << std::endl;
//            }
//            for (auto it = it1, iend = m_antiable_hashset2->cend(); it != iend; ++it) {
//              sum1 += *selected * *it;
//              oss2 << ' ' << *it << "@[" << it.creation_epoch() << ',' << it.deletion_epoch() << ')';
//            }
//            if (sum0 != sum1) {
//              m_debug_output->push(oss.str() + " :: " + oss2.str());
//              break;
//            }
//#endif
//            m_sum.fetch_sub(sum0, std::memory_order_relaxed);
//            if (erasure_epoch)
//              m_epoch_list->try_release(erasure_epoch);
//            else
//              std::cerr << 'y';
//          }
//#ifdef DEBUG_HARD
//          m_debug_output->push(oss.str());
//#endif
//          m_values_to_release.erase(selected);
//        }
//      }
//    }
//
//  private:
//    std::shared_ptr<Zeni::Concurrency::Epoch_List> m_epoch_list;
//    std::shared_ptr<Zeni::Concurrency::Antiable_Hashset<int64_t>> m_antiable_hashset1;
//    std::shared_ptr<Zeni::Concurrency::Antiable_Hashset<int64_t>> m_antiable_hashset2;
//    uint64_t m_to_acquire;
//#ifdef DEBUG_HARD
//    std::shared_ptr<Zeni::Concurrency::Queue<std::string>> m_debug_output;
//#endif
//    ZENI_CONCURRENCY_CACHE_ALIGN std::atomic_int64_t &m_sum;
//    std::vector<uint64_t> m_values_to_acquire;
//    std::vector<uint64_t> m_values_to_release;
//    std::random_device rd;
//    std::default_random_engine dre;
//  };
//
//  const auto epoch_list = std::make_shared<Zeni::Concurrency::Epoch_List>();
//  const auto antiable_hashset1 = std::make_shared<Zeni::Concurrency::Antiable_Hashset<int64_t>>();
//  const auto antiable_hashset2 = std::make_shared<Zeni::Concurrency::Antiable_Hashset<int64_t>>();
//#ifdef DEBUG_HARD
//  const auto debug_output = std::make_shared<Zeni::Concurrency::Queue<std::string>>();
//#endif
//  ZENI_CONCURRENCY_CACHE_ALIGN std::atomic_int64_t sum = 0;
//
//  std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
//  for (uint64_t i = 0; i != std::thread::hardware_concurrency() / 2; ++i) {
//    jobs.emplace_back(std::make_shared<Antiable>(epoch_list, antiable_hashset1, antiable_hashset1, 4,
//#ifdef DEBUG_HARD
//      debug_output,
//#endif
//      sum));
//    jobs.emplace_back(std::make_shared<Antiable>(epoch_list, antiable_hashset1, antiable_hashset1, 4,
//#ifdef DEBUG_HARD
//      debug_output,
//#endif
//      sum));
//  }
//  job_queue->give_many(std::move(jobs));
//
//  worker_threads->finish_jobs();
//
//  if (antiable_hashset1->size() != 0 || antiable_hashset1->usage() != 0 || antiable_hashset2->size() != 0 || antiable_hashset2->usage() != 0)
//    std::cerr << "X: " << antiable_hashset1->size() << ' ' << antiable_hashset1->usage() << ' ' << antiable_hashset2->size() << ' ' << antiable_hashset2->usage() << std::endl;
//  if (sum.load(std::memory_order_relaxed) != 0) {
//#ifdef DEBUG_HARD
//    std::cerr << std::endl;
//    std::string line;
//    while (debug_output->try_pop(line))
//      std::cerr << line << std::endl;
//    std::cerr << sum << std::endl;
//#else
//    std::cerr << 'Y';
//#endif
//    abort();
//  }
//
//  //std::cout << std::endl;
//}

//void test_Ctrie(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue) {
//
//  class Ctrier : public Zeni::Concurrency::Job {
//  public:
//    Ctrier(const std::shared_ptr<Zeni::Concurrency::Ctrie<uint64_t, const char *>> &ctrie) : m_ctrie(ctrie), dre(rd()) {}
//
//    void execute() noexcept override {
//      while (m_to_insert) {
//        const uint64_t value = std::uniform_int_distribution<uint64_t>(1, 10000)(dre);
//        m_ctrie->insert(value, nullptr);
//        --m_to_insert;
//      }
//      while (m_to_lookup) {
//        const uint64_t value = std::uniform_int_distribution<uint64_t>(1, 10000)(dre);
//        m_ctrie->lookup(value);
//        --m_to_lookup;
//      }
//      while (m_to_remove) {
//        const uint64_t value = std::uniform_int_distribution<uint64_t>(1, 10000)(dre);
//        m_ctrie->remove(value);
//        --m_to_remove;
//      }
//    }
//
//  private:
//    std::shared_ptr<Zeni::Concurrency::Ctrie<uint64_t, const char *>> m_ctrie;
//    int64_t m_to_insert = 1024;
//    int64_t m_to_lookup = 1024;
//    int64_t m_to_remove = 1024;
//    std::random_device rd;
//    std::default_random_engine dre;
//  };
//
//  const auto ctrie = std::make_shared<Zeni::Concurrency::Ctrie<uint64_t, const char *>>();
//
//  std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
//  for (uint64_t i = 0; i != std::thread::hardware_concurrency(); ++i)
//    jobs.emplace_back(std::make_shared<Ctrier>(ctrie));
//  job_queue->give_many(std::move(jobs));
//
//  worker_threads->finish_jobs();
//}

//void test_Hash_Trie(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue) {
//
//  class Hash_Trier : public Zeni::Concurrency::Job {
//  public:
//    Hash_Trier(const std::shared_ptr<Zeni::Concurrency::Hash_Trie<uint64_t, const char *>> &hash_trie) : m_hash_trie(hash_trie), dre(rd()) {}
//
//    void execute() noexcept override {
//      while (m_to_insert) {
//        const uint64_t value = std::uniform_int_distribution<uint64_t>(1, 10000)(dre);
//        m_hash_trie->insert_snapshot(value, nullptr);
//        --m_to_insert;
//      }
//      while (m_to_lookup) {
//        const uint64_t value = std::uniform_int_distribution<uint64_t>(1, 10000)(dre);
//        const auto[found, snapshot] = m_hash_trie->lookup_snapshot(value);
//        for (const auto &key_value : snapshot) {
//        }
//        --m_to_lookup;
//      }
//      while (m_to_remove) {
//        const uint64_t value = std::uniform_int_distribution<uint64_t>(1, 10000)(dre);
//        m_hash_trie->remove_snapshot(value);
//        --m_to_remove;
//      }
//    }
//
//  private:
//    std::shared_ptr<Zeni::Concurrency::Hash_Trie<uint64_t, const char *>> m_hash_trie;
//    int64_t m_to_insert = 1024;
//    int64_t m_to_lookup = 1024;
//    int64_t m_to_remove = 1024;
//    std::random_device rd;
//    std::default_random_engine dre;
//  };
//
//  const auto hash_trie = std::make_shared<Zeni::Concurrency::Hash_Trie<uint64_t, const char *>>();
//
//  std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
//  for (uint64_t i = 0; i != std::thread::hardware_concurrency(); ++i)
//    jobs.emplace_back(std::make_shared<Hash_Trier>(hash_trie));
//  job_queue->give_many(std::move(jobs));
//
//  worker_threads->finish_jobs();
//}

//#define DEBUG_HARD

//struct Null_Hash {
//  template <typename TYPE>
//  size_t operator()(TYPE &&) const {
//    return 0;
//  }
//};

template <size_t MINE, size_t THEIRS>
class Antiable : public Zeni::Concurrency::Job {
public:
  Antiable(const std::shared_ptr<Zeni::Concurrency::Super_Hash_Trie<Zeni::Concurrency::Antiable_Hash_Trie<int64_t>, Zeni::Concurrency::Antiable_Hash_Trie<int64_t>>> &antiable_hash_tries,
    const int64_t to_acquire,
#ifdef DEBUG_HARD
    const std::shared_ptr<Zeni::Concurrency::Queue<std::string>> &debug_output,
#endif
    std::atomic_int64_t &sum)
    : m_antiable_hash_tries(antiable_hash_tries),
    m_to_acquire(to_acquire),
#ifdef DEBUG_HARD
    m_debug_output(debug_output),
#endif
    m_sum(sum),
    dre(rd())
  {
  }

  void execute() noexcept override {
    //#ifdef DEBUG_HARD
    //      std::vector<Zeni::Concurrency::Antiable_Hash_Trie<int64_t, Null_Hash>::Snapshot> snapshots;
    //#endif
    m_values_to_acquire.reserve(m_to_acquire);
    m_values_to_release.reserve(m_to_acquire);
    for (int64_t i = 1; i != m_to_acquire + 1; ++i) {
      m_values_to_acquire.push_back(i);
      m_values_to_release.push_back(i);
    }
    while (!m_values_to_acquire.empty() || !m_values_to_release.empty()) {
      const size_t index = std::uniform_int_distribution<size_t>(0, m_values_to_acquire.size() + m_values_to_release.size() - 1)(dre);
      if (index < m_values_to_acquire.size()) {
        auto selected = m_values_to_acquire.begin();
        std::advance(selected, index);
#ifdef DEBUG_HARD
        std::ostringstream oss, oss2;
        oss << std::this_thread::get_id() << " +" << *selected << ':';
#endif
        const auto[first, snapshot] = m_antiable_hash_tries->template insert<MINE>(*selected);
        //snapshots.push_back(snapshot);
        if (first) {
          int64_t sum = 0;
          const auto theirs = snapshot.template snapshot<THEIRS>();
          for (const auto &value : theirs) {
            sum += *selected * value.key;
#ifdef DEBUG_HARD
            oss << ' ' << value.key;
#endif
          }
          m_sum.fetch_add(sum, std::memory_order_relaxed);
        }
#ifdef DEBUG_HARD
        m_debug_output->push(oss.str());
#endif
        m_values_to_acquire.erase(selected);
      }
      else {
        auto selected = m_values_to_release.begin();
        std::advance(selected, index - m_values_to_acquire.size());
#ifdef DEBUG_HARD
        std::ostringstream oss, oss2;
        oss << std::this_thread::get_id() << " -" << *selected << ':';
#endif
        const auto[last, snapshot] = m_antiable_hash_tries->template erase<MINE>(*selected);
        //snapshots.push_back(snapshot);
        if (last) {
          int64_t sum = 0;
          const auto theirs = snapshot.template snapshot<THEIRS>();
          for (const auto &value : theirs) {
            sum += *selected * value.key;
#ifdef DEBUG_HARD
            oss << ' ' << value.key;
#endif
          }
          m_sum.fetch_sub(sum, std::memory_order_relaxed);
        }
#ifdef DEBUG_HARD
        m_debug_output->push(oss.str());
#endif
        m_values_to_release.erase(selected);
      }
    }
  }

private:
  std::shared_ptr<Zeni::Concurrency::Super_Hash_Trie<Zeni::Concurrency::Antiable_Hash_Trie<int64_t>, Zeni::Concurrency::Antiable_Hash_Trie<int64_t>>> m_antiable_hash_tries;
  int64_t m_to_acquire;
#ifdef DEBUG_HARD
  std::shared_ptr<Zeni::Concurrency::Queue<std::string>> m_debug_output;
#endif
  ZENI_CONCURRENCY_CACHE_ALIGN std::atomic_int64_t &m_sum;
  std::vector<int64_t> m_values_to_acquire;
  std::vector<int64_t> m_values_to_release;
  std::random_device rd;
  std::default_random_engine dre;
};

void test_Antiable_Hash_Trie(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue) {
  const auto antiable_hash_tries = std::make_shared<Zeni::Concurrency::Super_Hash_Trie<Zeni::Concurrency::Antiable_Hash_Trie<int64_t>, Zeni::Concurrency::Antiable_Hash_Trie<int64_t>>>();
#ifdef DEBUG_HARD
  const auto debug_output = std::make_shared<Zeni::Concurrency::Queue<std::string>>();
#endif
  ZENI_CONCURRENCY_CACHE_ALIGN std::atomic_int64_t sum = 0;

  std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
  for (uint64_t i = 0; i != std::thread::hardware_concurrency() / 2; ++i) {
    jobs.emplace_back(std::make_shared<Antiable<0, 1>>(antiable_hash_tries, 256,
#ifdef DEBUG_HARD
      debug_output,
#endif
      sum));
    jobs.emplace_back(std::make_shared<Antiable<1, 0>>(antiable_hash_tries, 256,
#ifdef DEBUG_HARD
      debug_output,
#endif
      sum));
  }
  job_queue->give_many(std::move(jobs));

  worker_threads->finish_jobs();

  if (sum.load(std::memory_order_relaxed) != 0) {
#ifdef DEBUG_HARD
    std::cerr << std::endl;
    std::string line;
    while (debug_output->try_pop(line))
      std::cerr << line << std::endl;
    std::cerr << sum << std::endl;
#else
    std::cerr << 'Y';
#endif
    abort();
  }

  //std::cout << std::endl;
}

void test_Rete_Network(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue) {
  const auto network = Zeni::Rete::Network::Create(Zeni::Rete::Network::Printed_Output::None, worker_threads);

  for (int i = 0; i != 101; ++i) {
    std::array<std::shared_ptr<const Zeni::Rete::Symbol>, 5> symbols = {
      {
        std::make_shared<Zeni::Rete::Symbol_Constant_Int>(1),
        std::make_shared<Zeni::Rete::Symbol_Constant_Int>(2),
        std::make_shared<Zeni::Rete::Symbol_Constant_Int>(3),
        std::make_shared<Zeni::Rete::Symbol_Constant_Int>(4),
        std::make_shared<Zeni::Rete::Symbol_Constant_Int>(5)
      }
    };

    {
      auto filter0 = Zeni::Rete::Node_Filter::Create(network->get(), job_queue, Zeni::Rete::WME(symbols[0], symbols[0], symbols[0]));
      //auto passthrough0 = Zeni::Rete::Node_Passthrough::Create(network->get(), job_queue, filter0);
      auto filter1 = Zeni::Rete::Node_Filter::Create(network->get(), job_queue, Zeni::Rete::WME(symbols[0], symbols[0], symbols[1]));
      auto unary_gate1 = Zeni::Rete::Node_Unary_Gate::Create(network->get(), job_queue, filter1);
      auto gated_passthrough01 = Zeni::Rete::Node_Passthrough_Gated::Create(network->get(), job_queue, filter0, unary_gate1);
      auto action = Zeni::Rete::Node_Action::Create(network->get(), job_queue, "hello-world", false, gated_passthrough01, Zeni::Rete::Variable_Indices::Create(),
        [](const Zeni::Rete::Node_Action &, const Zeni::Rete::Token &) {
        std::cout << '(' << std::flush;
      }, [](const Zeni::Rete::Node_Action &, const Zeni::Rete::Token &) {
        std::cout << ')' << std::flush;
      });

      //job_queue->give_one(
      //  std::make_shared<Zeni::Rete::Message_Decrement_Output_Count>(gated_passthrough1, network->get(), gated_passthrough1));
      //job_queue->give_one(
      //  std::make_shared<Zeni::Rete::Message_Disconnect_Output>(network->get(), network->get(), filter1, true));
    }

    //(*network)->get_Worker_Threads()->finish_jobs();

    (*network)->insert_wme(job_queue, std::make_shared<Zeni::Rete::WME>(symbols[0], symbols[0], symbols[0]));

    //(*network)->get_Worker_Threads()->finish_jobs();

    (*network)->insert_wme(job_queue, std::make_shared<Zeni::Rete::WME>(symbols[0], symbols[0], symbols[1]));

    //(*network)->get_Worker_Threads()->finish_jobs();

    (*network)->excise_rule(job_queue, "hello-world", false);

    //(*network)->get_Worker_Threads()->finish_jobs();

    {
      auto filter0 = Zeni::Rete::Node_Filter::Create(network->get(), job_queue, Zeni::Rete::WME(symbols[0], symbols[0], symbols[0]));
      //auto passthrough0 = Zeni::Rete::Node_Passthrough::Create(network->get(), job_queue, filter0);
      auto filter1 = Zeni::Rete::Node_Filter::Create(network->get(), job_queue, Zeni::Rete::WME(symbols[0], symbols[0], symbols[1]));
      auto unary_gate0 = Zeni::Rete::Node_Unary_Gate::Create(network->get(), job_queue, filter0);
      auto gated_passthrough10 = Zeni::Rete::Node_Passthrough_Gated::Create(network->get(), job_queue, filter1, unary_gate0);
      auto action = Zeni::Rete::Node_Action::Create(network->get(), job_queue, "gday-world", false, gated_passthrough10, Zeni::Rete::Variable_Indices::Create(),
        [](const Zeni::Rete::Node_Action &, const Zeni::Rete::Token &) {
        std::cout << '[' << std::flush;
      }, [](const Zeni::Rete::Node_Action &, const Zeni::Rete::Token &) {
        std::cout << ']' << std::flush;
      });

      //job_queue->give_one(
      //  std::make_shared<Zeni::Rete::Message_Decrement_Output_Count>(gated_passthrough1, network->get(), gated_passthrough1));
    }

    //(*network)->get_Worker_Threads()->finish_jobs();

    (*network)->remove_wme(job_queue, std::make_shared<Zeni::Rete::WME>(symbols[0], symbols[0], symbols[0]));

    //(*network)->get_Worker_Threads()->finish_jobs();

    (*network)->remove_wme(job_queue, std::make_shared<Zeni::Rete::WME>(symbols[0], symbols[0], symbols[1]));

    //(*network)->get_Worker_Threads()->finish_jobs();

    //(*network)->excise_all(job_queue);
  }
}

void test_Parser(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue) {
  const auto network = Zeni::Rete::Network::Create(Zeni::Rete::Network::Printed_Output::None, worker_threads);

  auto parser = Zeni::Rete::Parser::Create();

  parser->parse_string(network->get(), job_queue, "sp {test-rule\r\n  (<s> ^attr 42)\r\n  (<s> ^attr 3.14159)\r\n-->\r\n}\r\n", true);

  (*network)->insert_wme(job_queue, std::make_shared<Zeni::Rete::WME>(
    std::make_shared<Zeni::Rete::Symbol_Identifier>("S1"),
    std::make_shared<Zeni::Rete::Symbol_Constant_String>("attr"),
    std::make_shared<Zeni::Rete::Symbol_Constant_Int>(42)));

  (*network)->get_Worker_Threads()->finish_jobs();

  //(*network)->remove_wme(job_queue, std::make_shared<Zeni::Rete::WME>(
  //  std::make_shared<Zeni::Rete::Symbol_Identifier>("S1"),
  //  std::make_shared<Zeni::Rete::Symbol_Constant_String>("attr"),
  //  std::make_shared<Zeni::Rete::Symbol_Constant_Int>(42)));

  //(*network)->get_Worker_Threads()->finish_jobs();
}
