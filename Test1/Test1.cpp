#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>
#endif

#include "Zeni/Concurrency/Container/Antiable_Hash_Trie.hpp"
#include "Zeni/Concurrency/Container/Hash_Trie.hpp"
#include "Zeni/Concurrency/Container/Positive_Hash_Trie.hpp"
#include "Zeni/Concurrency/Container/Super_Hash_Trie.hpp"
//#include "Zeni/Concurrency/Container/Ctrie.hpp"
#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Concurrency/Message.hpp"
#include "Zeni/Concurrency/Mutex.hpp"
#include "Zeni/Concurrency/Recipient.hpp"
#include "Zeni/Concurrency/Worker_Threads.hpp"

#include "Zeni/Rete/Internal/Debug_Counters.hpp"

//#include "Zeni/Rete/Network.hpp"
//#include "Zeni/Rete/Node_Action.hpp"
//#include "Zeni/Rete/Node_Filter.hpp"
//#include "Zeni/Rete/Node_Passthrough_Gated.hpp"
//#include "Zeni/Rete/Node_Unary_Gate.hpp"
//#include "Zeni/Rete/Parser.hpp"

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

//static void test_Ctrie(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_Antiable_Hash_Trie(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_Hash_Trie(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_Positive_Hash_Trie(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
//static void test_Rete_Network(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
//static void test_Parser(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);

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

  //for (int i = 0; i != 80000; ++i) {
  //  test_Ctrie(worker_threads, job_queue);
  //  //if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
  //  //  std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
  //  //  abort();
  //  //}
  //  std::cout << 'C' << std::flush;
  //}
  //std::cout << std::endl;

  for (int i = 0; i != 80; ++i) {
    test_Antiable_Hash_Trie(worker_threads, job_queue);
    //if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
    //  std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
    //  abort();
    //}
    std::cout << 'A' << std::flush;
  }
  std::cout << std::endl;

  for (int i = 0; i != 80; ++i) {
    test_Hash_Trie(worker_threads, job_queue);
    //if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
    //  std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
    //  abort();
    //}
    std::cout << 'H' << std::flush;
  }
  std::cout << std::endl;

  for (int i = 0; i != 80; ++i) {
    test_Positive_Hash_Trie(worker_threads, job_queue);
    //if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
    //  std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
    //  abort();
    //}
    std::cout << 'P' << std::flush;
  }
  std::cout << std::endl;

  //Zeni::Rete::Debug_Counters::print(std::cerr);
  //test_Rete_Network(worker_threads, job_queue);
  //Zeni::Rete::Debug_Counters::print(std::cerr);
  ////if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
  ////  std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
  ////  abort();
  ////}
  //Zeni::Rete::Debug_Counters::reset();

  std::cerr << "Test: ";
  for (int i = 1; i != 11; ++i) {
    std::cerr << ' ' << i;
    //test_Rete_Network(worker_threads, job_queue);
    //if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
    //  std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
    //  Zeni::Rete::Debug_Counters::print(std::cerr);
    //  abort();
    //}
    Zeni::Rete::Debug_Counters::reset();
  }

  std::cerr << std::endl;

  //test_Parser(worker_threads, job_queue);
  //if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
  //  std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
  //  Zeni::Rete::Debug_Counters::print(std::cerr);
  //  abort();
  //}

  return 0;
}

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
        const auto[result, snapshot, inserted] = m_antiable_hash_tries->template insert<MINE>(*selected);
        //snapshots.push_back(snapshot);
        if (result == Zeni::Concurrency::Antiable_Hash_Trie<int64_t>::Result::First_Insertion) {
          int64_t sum = 0;
          const auto theirs = snapshot.template snapshot<THEIRS>();
          for (const auto &value : theirs) {
            sum += *selected * value;
#ifdef DEBUG_HARD
            oss << ' ' << value;
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
        const auto[result, snapshot, erased] = m_antiable_hash_tries->template erase<MINE>(*selected);
        //snapshots.push_back(snapshot);
        if (result == Zeni::Concurrency::Antiable_Hash_Trie<int64_t>::Result::Last_Removal) {
          int64_t sum = 0;
          const auto theirs = snapshot.template snapshot<THEIRS>();
          for (const auto &value : theirs) {
            sum += *selected * value;
#ifdef DEBUG_HARD
            oss << ' ' << value;
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
    jobs.emplace_back(std::shared_ptr<Antiable<0, 1>>(new Antiable<0, 1>(antiable_hash_tries, 256,
#ifdef DEBUG_HARD
      debug_output,
#endif
      sum)));
    jobs.emplace_back(std::shared_ptr<Antiable<1, 0>>(new Antiable<1, 0>(antiable_hash_tries, 256,
#ifdef DEBUG_HARD
      debug_output,
#endif
      sum)));
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

template <size_t MINE, size_t THEIRS>
class Normal : public Zeni::Concurrency::Job {
public:
  Normal(const std::shared_ptr<Zeni::Concurrency::Super_Hash_Trie<Zeni::Concurrency::Hash_Trie<int64_t>, Zeni::Concurrency::Hash_Trie<int64_t>>> &hash_tries,
    const int64_t to_acquire,
    const int64_t count,
#ifdef DEBUG_HARD
    const std::shared_ptr<Zeni::Concurrency::Queue<std::string>> &debug_output,
#endif
    std::atomic_int64_t &sum)
    : m_hash_tries(hash_tries),
    m_to_acquire(to_acquire),
    m_count(count),
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
    for (int64_t i = 1; i != m_to_acquire + 1; ++i)
      m_values_to_acquire.push_back(m_count * m_to_acquire + i);
    while (!m_values_to_acquire.empty() || !m_values_to_release.empty()) {
      const size_t index = std::uniform_int_distribution<size_t>(0, m_values_to_acquire.size() + m_values_to_release.size() - 1)(dre);
      if (index < m_values_to_acquire.size()) {
        auto selected = m_values_to_acquire.begin();
        std::advance(selected, index);
#ifdef DEBUG_HARD
        std::ostringstream oss, oss2;
        oss << std::this_thread::get_id() << " +" << *selected << ':';
#endif
        const auto[result, snapshot, inserted, replaced] = m_hash_tries->template insert<MINE>(*selected);
        //snapshots.push_back(snapshot);
        if (result == Zeni::Concurrency::Hash_Trie<int64_t>::Result::First_Insertion) {
          int64_t sum = 0;
          const auto theirs = snapshot.template snapshot<THEIRS>();
          for (const auto &value : theirs) {
            sum += *selected * value;
#ifdef DEBUG_HARD
            oss << ' ' << value;
#endif
          }
          m_sum.fetch_add(sum, std::memory_order_relaxed);
        }
#ifdef DEBUG_HARD
        m_debug_output->push(oss.str());
#endif
        m_values_to_release.push_back(*selected);
        m_values_to_acquire.erase(selected);
      }
      else {
        auto selected = m_values_to_release.begin();
        std::advance(selected, index - m_values_to_acquire.size());
#ifdef DEBUG_HARD
        std::ostringstream oss, oss2;
        oss << std::this_thread::get_id() << " -" << *selected << ':';
#endif
        const auto[result, snapshot, erased] = m_hash_tries->template erase<MINE>(*selected);
        //snapshots.push_back(snapshot);
        if (result == Zeni::Concurrency::Hash_Trie<int64_t>::Result::Last_Removal) {
          int64_t sum = 0;
          const auto theirs = snapshot.template snapshot<THEIRS>();
          for (const auto &value : theirs) {
            sum += *selected * value;
#ifdef DEBUG_HARD
            oss << ' ' << value;
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
  std::shared_ptr<Zeni::Concurrency::Super_Hash_Trie<Zeni::Concurrency::Hash_Trie<int64_t>, Zeni::Concurrency::Hash_Trie<int64_t>>> m_hash_tries;
  int64_t m_to_acquire;
  int64_t m_count;
#ifdef DEBUG_HARD
  std::shared_ptr<Zeni::Concurrency::Queue<std::string>> m_debug_output;
#endif
  ZENI_CONCURRENCY_CACHE_ALIGN std::atomic_int64_t &m_sum;
  std::vector<int64_t> m_values_to_acquire;
  std::vector<int64_t> m_values_to_release;
  std::random_device rd;
  std::default_random_engine dre;
};

void test_Hash_Trie(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue) {
  const auto hash_tries = std::make_shared<Zeni::Concurrency::Super_Hash_Trie<Zeni::Concurrency::Hash_Trie<int64_t>, Zeni::Concurrency::Hash_Trie<int64_t>>>();
#ifdef DEBUG_HARD
  const auto debug_output = std::make_shared<Zeni::Concurrency::Queue<std::string>>();
#endif
  ZENI_CONCURRENCY_CACHE_ALIGN std::atomic_int64_t sum = 0;

  std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
  for (uint64_t i = 0; i != std::thread::hardware_concurrency() / 2; ++i) {
    jobs.emplace_back(std::shared_ptr<Normal<0, 1>>(new Normal<0, 1>(hash_tries, 256, i,
#ifdef DEBUG_HARD
      debug_output,
#endif
      sum)));
    jobs.emplace_back(std::shared_ptr<Normal<1, 0>>(new Normal<1, 0>(hash_tries, 256, i,
#ifdef DEBUG_HARD
      debug_output,
#endif
      sum)));
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

template <size_t MINE, size_t THEIRS>
class Positive : public Zeni::Concurrency::Job {
public:
  Positive(const std::shared_ptr<Zeni::Concurrency::Super_Hash_Trie<Zeni::Concurrency::Positive_Hash_Trie<int64_t>, Zeni::Concurrency::Positive_Hash_Trie<int64_t>>> &positive_hash_tries,
    const int64_t to_acquire,
#ifdef DEBUG_HARD
    const std::shared_ptr<Zeni::Concurrency::Queue<std::string>> &debug_output,
#endif
    std::atomic_int64_t &sum)
    : m_positive_hash_tries(positive_hash_tries),
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
    for (int64_t i = 1; i != m_to_acquire + 1; ++i)
      m_values_to_acquire.push_back(i);
    while (!m_values_to_acquire.empty() || !m_values_to_release.empty()) {
      const size_t index = std::uniform_int_distribution<size_t>(0, m_values_to_acquire.size() + m_values_to_release.size() - 1)(dre);
      if (index < m_values_to_acquire.size()) {
        auto selected = m_values_to_acquire.begin();
        std::advance(selected, index);
#ifdef DEBUG_HARD
        std::ostringstream oss, oss2;
        oss << std::this_thread::get_id() << " +" << *selected << ':';
#endif
        const auto[result, snapshot, inserted] = m_positive_hash_tries->template insert<MINE>(*selected);
        //snapshots.push_back(snapshot);
        if (result == Zeni::Concurrency::Positive_Hash_Trie<int64_t>::Result::First_Insertion) {
          int64_t sum = 0;
          const auto theirs = snapshot.template snapshot<THEIRS>();
          for (const auto &value : theirs) {
            sum += *selected * value;
#ifdef DEBUG_HARD
            oss << ' ' << value;
#endif
          }
          m_sum.fetch_add(sum, std::memory_order_relaxed);
        }
#ifdef DEBUG_HARD
        m_debug_output->push(oss.str());
#endif
        m_values_to_release.push_back(*selected);
        m_values_to_acquire.erase(selected);
      }
      else {
        auto selected = m_values_to_release.begin();
        std::advance(selected, index - m_values_to_acquire.size());
#ifdef DEBUG_HARD
        std::ostringstream oss, oss2;
        oss << std::this_thread::get_id() << " -" << *selected << ':';
#endif
        const auto[result, snapshot, erased] = m_positive_hash_tries->template erase<MINE>(*selected);
        //snapshots.push_back(snapshot);
        if (result == Zeni::Concurrency::Positive_Hash_Trie<int64_t>::Result::Last_Removal) {
          int64_t sum = 0;
          const auto theirs = snapshot.template snapshot<THEIRS>();
          for (const auto &value : theirs) {
            sum += *selected * value;
#ifdef DEBUG_HARD
            oss << ' ' << value;
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
  std::shared_ptr<Zeni::Concurrency::Super_Hash_Trie<Zeni::Concurrency::Positive_Hash_Trie<int64_t>, Zeni::Concurrency::Positive_Hash_Trie<int64_t>>> m_positive_hash_tries;
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

void test_Positive_Hash_Trie(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue) {
  const auto positive_hash_tries = std::make_shared<Zeni::Concurrency::Super_Hash_Trie<Zeni::Concurrency::Positive_Hash_Trie<int64_t>, Zeni::Concurrency::Positive_Hash_Trie<int64_t>>>();
#ifdef DEBUG_HARD
  const auto debug_output = std::make_shared<Zeni::Concurrency::Queue<std::string>>();
#endif
  ZENI_CONCURRENCY_CACHE_ALIGN std::atomic_int64_t sum = 0;

  std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
  for (uint64_t i = 0; i != std::thread::hardware_concurrency() / 2; ++i) {
    jobs.emplace_back(std::shared_ptr<Positive<0, 1>>(new Positive<0, 1>(positive_hash_tries, 256,
#ifdef DEBUG_HARD
      debug_output,
#endif
      sum)));
    jobs.emplace_back(std::shared_ptr<Positive<1, 0>>(new Positive<1, 0>(positive_hash_tries, 256,
#ifdef DEBUG_HARD
      debug_output,
#endif
      sum)));
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

//void test_Rete_Network(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue) {
//  const auto network = Zeni::Rete::Network::Create(Zeni::Rete::Network::Printed_Output::None, worker_threads);
//
//  for (int i = 0; i != 101; ++i) {
//    std::array<std::shared_ptr<const Zeni::Rete::Symbol>, 5> symbols = {
//      {
//        std::make_shared<Zeni::Rete::Symbol_Constant_Int>(1),
//        std::make_shared<Zeni::Rete::Symbol_Constant_Int>(2),
//        std::make_shared<Zeni::Rete::Symbol_Constant_Int>(3),
//        std::make_shared<Zeni::Rete::Symbol_Constant_Int>(4),
//        std::make_shared<Zeni::Rete::Symbol_Constant_Int>(5)
//      }
//    };
//
//    {
//      auto filter0 = Zeni::Rete::Node_Filter::Create(network->get(), job_queue, Zeni::Rete::WME(symbols[0], symbols[0], symbols[0]));
//      //auto passthrough0 = Zeni::Rete::Node_Passthrough::Create(network->get(), job_queue, filter0);
//      auto filter1 = Zeni::Rete::Node_Filter::Create(network->get(), job_queue, Zeni::Rete::WME(symbols[0], symbols[0], symbols[1]));
//      auto unary_gate1 = Zeni::Rete::Node_Unary_Gate::Create(network->get(), job_queue, filter1);
//      auto gated_passthrough01 = Zeni::Rete::Node_Passthrough_Gated::Create(network->get(), job_queue, filter0, unary_gate1);
//      auto action = Zeni::Rete::Node_Action::Create(network->get(), job_queue, "hello-world", false, gated_passthrough01, Zeni::Rete::Variable_Indices::Create(),
//        [](const Zeni::Rete::Node_Action &, const Zeni::Rete::Token &) {
//        std::cout << '(' << std::flush;
//      }, [](const Zeni::Rete::Node_Action &, const Zeni::Rete::Token &) {
//        std::cout << ')' << std::flush;
//      });
//
//      //job_queue->give_one(
//      //  std::make_shared<Zeni::Rete::Message_Decrement_Output_Count>(gated_passthrough1, network->get(), gated_passthrough1));
//      //job_queue->give_one(
//      //  std::make_shared<Zeni::Rete::Message_Disconnect_Output>(network->get(), network->get(), filter1, true));
//    }
//
//    //(*network)->get_Worker_Threads()->finish_jobs();
//
//    (*network)->insert_wme(job_queue, std::make_shared<Zeni::Rete::WME>(symbols[0], symbols[0], symbols[0]));
//
//    //(*network)->get_Worker_Threads()->finish_jobs();
//
//    (*network)->insert_wme(job_queue, std::make_shared<Zeni::Rete::WME>(symbols[0], symbols[0], symbols[1]));
//
//    //(*network)->get_Worker_Threads()->finish_jobs();
//
//    (*network)->excise_rule(job_queue, "hello-world", false);
//
//    //(*network)->get_Worker_Threads()->finish_jobs();
//
//    {
//      auto filter0 = Zeni::Rete::Node_Filter::Create(network->get(), job_queue, Zeni::Rete::WME(symbols[0], symbols[0], symbols[0]));
//      //auto passthrough0 = Zeni::Rete::Node_Passthrough::Create(network->get(), job_queue, filter0);
//      auto filter1 = Zeni::Rete::Node_Filter::Create(network->get(), job_queue, Zeni::Rete::WME(symbols[0], symbols[0], symbols[1]));
//      auto unary_gate0 = Zeni::Rete::Node_Unary_Gate::Create(network->get(), job_queue, filter0);
//      auto gated_passthrough10 = Zeni::Rete::Node_Passthrough_Gated::Create(network->get(), job_queue, filter1, unary_gate0);
//      auto action = Zeni::Rete::Node_Action::Create(network->get(), job_queue, "gday-world", false, gated_passthrough10, Zeni::Rete::Variable_Indices::Create(),
//        [](const Zeni::Rete::Node_Action &, const Zeni::Rete::Token &) {
//        std::cout << '[' << std::flush;
//      }, [](const Zeni::Rete::Node_Action &, const Zeni::Rete::Token &) {
//        std::cout << ']' << std::flush;
//      });
//
//      //job_queue->give_one(
//      //  std::make_shared<Zeni::Rete::Message_Decrement_Output_Count>(gated_passthrough1, network->get(), gated_passthrough1));
//    }
//
//    //(*network)->get_Worker_Threads()->finish_jobs();
//
//    (*network)->remove_wme(job_queue, std::make_shared<Zeni::Rete::WME>(symbols[0], symbols[0], symbols[0]));
//
//    //(*network)->get_Worker_Threads()->finish_jobs();
//
//    (*network)->remove_wme(job_queue, std::make_shared<Zeni::Rete::WME>(symbols[0], symbols[0], symbols[1]));
//
//    //(*network)->get_Worker_Threads()->finish_jobs();
//
//    //(*network)->excise_all(job_queue);
//  }
//}
//
//void test_Parser(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue) {
//  const auto network = Zeni::Rete::Network::Create(Zeni::Rete::Network::Printed_Output::None, worker_threads);
//
//  auto parser = Zeni::Rete::Parser::Create();
//
//  parser->parse_string(network->get(), job_queue, "sp {test-rule\r\n  (<s> ^attr 42)\r\n  (<s> ^attr 3.14159)\r\n-->\r\n}\r\n", true);
//
//  (*network)->insert_wme(job_queue, std::make_shared<Zeni::Rete::WME>(
//    std::make_shared<Zeni::Rete::Symbol_Identifier>("S1"),
//    std::make_shared<Zeni::Rete::Symbol_Constant_String>("attr"),
//    std::make_shared<Zeni::Rete::Symbol_Constant_Int>(42)));
//
//  (*network)->get_Worker_Threads()->finish_jobs();
//
//  //(*network)->remove_wme(job_queue, std::make_shared<Zeni::Rete::WME>(
//  //  std::make_shared<Zeni::Rete::Symbol_Identifier>("S1"),
//  //  std::make_shared<Zeni::Rete::Symbol_Constant_String>("attr"),
//  //  std::make_shared<Zeni::Rete::Symbol_Constant_Int>(42)));
//
//  //(*network)->get_Worker_Threads()->finish_jobs();
//}
