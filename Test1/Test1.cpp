#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>
#endif

#include "Zeni/Concurrency/Container/Antiable_Hash_Trie.hpp"
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
#include <iostream>
#include <list>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>

static void test_Intrusive_Stack(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_Antiable_Hash_Trie(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_Hash_Trie(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_Positive_Hash_Trie(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_Rete_Network(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_Parser(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);

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

  //{
  //  Zeni::Concurrency::Super_Hash_Trie<
  //    Zeni::Concurrency::Hash_Trie_S2<int64_t, Zeni::Concurrency::Super_Hash_Trie<Zeni::Concurrency::Positive_Hash_Trie<int64_t>, Zeni::Concurrency::Positive_Hash_Trie<int64_t>>>,
  //    Zeni::Concurrency::Positive_Hash_Trie<int64_t>> tries;
  //
  //  {
  //    const auto result = tries.move_2<0, 0, 1>(13, int64_t(42));
  //  }
  //
  //  {
  //    const auto result = tries.insert_2<0, 0>(13, 42);
  //  }
  //
  //  {
  //    const auto result = tries.move_2<0, 0, 1>(13, int64_t(42));
  //  }
  //
  //  {
  //    const auto result = tries.move_2<0, 0, 1>(13, int64_t(42));
  //  }
  //
  //  {
  //    const auto result = tries.insert_2<0, 0>(13, int64_t(42));
  //  }
  //
  //  {
  //    const auto result = tries.move_2<0, 0, 1>(13, int64_t(42));
  //  }
  //}

  //{
  //    Zeni::Concurrency::Super_Hash_Trie<
  //      Zeni::Concurrency::Hash_Trie_S2<int64_t, Zeni::Concurrency::Super_Hash_Trie<Zeni::Concurrency::Positive_Hash_Trie<int64_t>, Zeni::Concurrency::Positive_Hash_Trie<int64_t>>>,
  //      Zeni::Concurrency::Positive_Hash_Trie<int64_t>> tries;
  //  
  //  {
  //    const auto result = tries.insert_2_ip_xp<0, 0, 1>(13, 42);
  //  }
  //
  //  {
  //    const auto result = tries.move_2<0, 1, 0>(13, int64_t(42));
  //  }
  //
  //  {
  //    const auto result = tries.insert_2_ip_xp<0, 0, 1>(13, 42);
  //  }
  //
  //  {
  //    const auto result = tries.insert_2_ip_xp<0, 1, 0>(13, 42);
  //  }
  //
  //  {
  //    const auto result = tries.erase_2_ip_xp<0, 0, 1>(13, 42);
  //  }
  //
  //  {
  //    const auto result = tries.move_2<0, 0, 1>(13, int64_t(42));
  //  }
  //
  //  {
  //    const auto result = tries.erase_2_ip_xp<0, 1, 0>(13, 42);
  //  }
  //
  //  {
  //    const auto result = tries.erase_2_ip_xp<0, 0, 1>(13, 42);
  //  }
  //
  //  {
  //    const auto result = tries.erase_2_ip_xp<0, 1, 0>(13, 42);
  //  }
  //}

  const auto worker_threads = Zeni::Concurrency::Worker_Threads::Create(g_num_cores);
  const auto job_queue = worker_threads->get_main_Job_Queue();

  //for (int i = 0; i != 80; ++i) {
  //  test_Intrusive_Stack(worker_threads, job_queue);
  //  //if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
  //  //  std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
  //  //  abort();
  //  //}
  //  std::cout << 'I' << std::flush;
  //}
  //std::cout << std::endl;

  //for (int i = 0; i != 80; ++i) {
  //  test_Antiable_Hash_Trie(worker_threads, job_queue);
  //  //if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
  //  //  std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
  //  //  abort();
  //  //}
  //  std::cout << 'A' << std::flush;
  //}
  //std::cout << std::endl;

  //for (int i = 0; i != 80; ++i) {
  //  test_Positive_Hash_Trie(worker_threads, job_queue);
  //  //if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
  //  //  std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
  //  //  abort();
  //  //}
  //  std::cout << 'P' << std::flush;
  //}
  //std::cout << std::endl;

  //for (int i = 0; i != 80; ++i) {
  //  test_Hash_Trie(worker_threads, job_queue);
  //  //if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
  //  //  std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
  //  //  abort();
  //  //}
  //  std::cout << 'H' << std::flush;
  //}
  //std::cout << std::endl;

  std::cerr << "Test: ";
  for (int i = 1; i != 11; ++i) {
    std::cerr << ' ' << i;
    test_Rete_Network(worker_threads, job_queue);
  }

  std::cerr << std::endl;

  test_Parser(worker_threads, job_queue);

  return 0;
}

//struct Null_Hash {
//  template <typename TYPE>
//  size_t operator()(TYPE &&) const {
//    return 0;
//  }
//};

void test_Intrusive_Stack(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue) {
  struct Int : public Zeni::Concurrency::Reclamation_Stack::Node, public Zeni::Concurrency::Intrusive_Stack<Int>::Node {
    Int() = default;
    Int(const int value_) : value(value_) {}

    int value = 0;
  };

  const auto stack = std::make_shared<Zeni::Concurrency::Intrusive_Stack<Int>>();

  class Job : public Zeni::Concurrency::Job {
    Job(const Job &) = delete;
    Job & operator=(const Job &) = delete;

  public:
    Job(const decltype(stack) stack_)
      : m_stack(stack_)
    {
    }

    void execute() noexcept override {
      for (int i = 0; i != 10000; ++i)
        m_stack->push(new Int(i));
      for (int i = 0; i != 10000; ++i) {
        if (const auto ptr = m_stack->try_pop())
          Zeni::Concurrency::Reclamation_Stack::push(ptr);
      }
    }

  private:
    decltype(stack) m_stack;
  };

  std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
  for (int16_t i = 0; i != g_num_cores; ++i)
    jobs.emplace_back(std::shared_ptr<Job>(new Job(stack)));
  job_queue->give_many(std::move(jobs));

  worker_threads->finish_jobs();
}

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
  for (int16_t i = 0; i != g_num_cores / 2; ++i) {
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
  for (int16_t i = 0; i != g_num_cores / 2; ++i) {
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
  for (int16_t i = 0; i != g_num_cores / 2; ++i) {
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

class Action_Print_Char : public Zeni::Rete::Node_Action::Action {
  Action_Print_Char(const Action_Print_Char &) = delete;
  Action_Print_Char & operator=(const Action_Print_Char &) = delete;

public:
  Action_Print_Char(const char c_)
    : c(c_)
  {
  }

  void operator()(const std::shared_ptr<Zeni::Rete::Network>, const std::shared_ptr<Zeni::Concurrency::Job_Queue>, const std::shared_ptr<Zeni::Rete::Node_Action>, const std::shared_ptr<const Zeni::Rete::Node_Action::Data>) const override {
    std::cout << c << std::flush;
  }

  const char c;
};

void test_Rete_Network(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue) {
  const auto network = Zeni::Rete::Network::Create(Zeni::Rete::Network::Printed_Output::None, worker_threads);

  for (int i = 1; i != 101; ++i) {
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
      auto filter01a = Zeni::Rete::Node_Filter_1::Create(network->get(), job_queue, Zeni::Rete::Node_Key_Symbol::Create(symbols[0]));
      auto filter02a = Zeni::Rete::Node_Filter_2::Create(network->get(), job_queue, Zeni::Rete::Node_Key_Symbol::Create(symbols[0]), filter01a);
      auto filter01b = Zeni::Rete::Node_Filter_1::Create(network->get(), job_queue, Zeni::Rete::Node_Key_Symbol::Create(symbols[0]));
      auto filter02b = Zeni::Rete::Node_Filter_2::Create(network->get(), job_queue, Zeni::Rete::Node_Key_Symbol::Create(symbols[0]), filter01b);
      auto join0 = Zeni::Rete::Node_Join::Create(network->get(), job_queue, Zeni::Rete::Node_Key_Symbol::Create(symbols[0]), Zeni::Rete::Node_Key_Symbol::Create(symbols[1]), filter02a, filter02b, { Zeni::Rete::Variable_Binding(Zeni::Rete::Token_Index(0, 0, 0), Zeni::Rete::Token_Index(0, 0, 0)) });
      auto action0 = Zeni::Rete::Node_Action::Create(network->get(), job_queue, "hello-world", false, Zeni::Rete::Node_Key_Null::Create(), join0, Zeni::Rete::Variable_Indices::Create(),
        std::make_shared<Action_Print_Char>('('), std::make_shared<Action_Print_Char>(')'));
    }

    //(*network)->get_Worker_Threads()->finish_jobs();

    (*network)->insert_wme(job_queue, std::make_shared<Zeni::Rete::WME>(symbols[0], symbols[0], symbols[0]));

    //(*network)->get_Worker_Threads()->finish_jobs();

    (*network)->insert_wme(job_queue, std::make_shared<Zeni::Rete::WME>(symbols[0], symbols[0], symbols[1]));

    //(*network)->get_Worker_Threads()->finish_jobs();

    (*network)->excise_rule(job_queue, "hello-world", false);

    //(*network)->get_Worker_Threads()->finish_jobs();

    {
      Zeni::Rete::Node_Key_Multisym::Node_Key_Symbol_Trie multisyms;
      multisyms.insert(Zeni::Rete::Node_Key_Symbol::Create(symbols[0]));
      multisyms.insert(Zeni::Rete::Node_Key_Symbol::Create(symbols[1]));
      multisyms.insert(Zeni::Rete::Node_Key_Symbol::Create(symbols[2]));
      const auto multikey = Zeni::Rete::Node_Key_Multisym::Create(std::move(multisyms));

      auto filter11a = Zeni::Rete::Node_Filter_1::Create(network->get(), job_queue, Zeni::Rete::Node_Key_Symbol::Create(symbols[0]));
      auto filter12a = Zeni::Rete::Node_Filter_2::Create(network->get(), job_queue, multikey, filter11a);
      auto filter11b = Zeni::Rete::Node_Filter_1::Create(network->get(), job_queue, multikey);
      auto filter12b = Zeni::Rete::Node_Filter_2::Create(network->get(), job_queue, Zeni::Rete::Node_Key_Symbol::Create(symbols[0]), filter11b);
      auto predicate = Zeni::Rete::Node_Predicate::Create(network->get(), job_queue, Zeni::Rete::Node_Key_Symbol::Create(symbols[1]), filter12b, Zeni::Rete::Node_Predicate::Predicate_E::Create(), Zeni::Rete::Token_Index(0, 0, 2), std::make_shared<Zeni::Rete::Node_Predicate::Get_Symbol_Constant>(symbols[1]));
      auto join1 = Zeni::Rete::Node_Join::Create(network->get(), job_queue, Zeni::Rete::Node_Key_Symbol::Create(symbols[0]), Zeni::Rete::Node_Key_Null::Create(), filter12a, predicate, { Zeni::Rete::Variable_Binding(Zeni::Rete::Token_Index(0, 0, 0), Zeni::Rete::Token_Index(0, 0, 0)) });
      auto action1 = Zeni::Rete::Node_Action::Create(network->get(), job_queue, "gday-world", false, Zeni::Rete::Node_Key_Null::Create(), join1, Zeni::Rete::Variable_Indices::Create(),
        std::make_shared<Action_Print_Char>('['), std::make_shared<Action_Print_Char>(']'));
    }

    //(*network)->get_Worker_Threads()->finish_jobs();

    (*network)->remove_wme(job_queue, std::make_shared<Zeni::Rete::WME>(symbols[0], symbols[0], symbols[0]));

    //(*network)->get_Worker_Threads()->finish_jobs();

    (*network)->remove_wme(job_queue, std::make_shared<Zeni::Rete::WME>(symbols[0], symbols[0], symbols[1]));

    //(*network)->get_Worker_Threads()->finish_jobs();

    (*network)->excise_all(job_queue, false);

    //(*network)->get_Worker_Threads()->finish_jobs();

    if(i % 100 == 0)
      (*network)->get_Worker_Threads()->finish_jobs();
  }
}

void test_Parser(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue) {
  const auto network = Zeni::Rete::Network::Create(Zeni::Rete::Network::Printed_Output::None, worker_threads);

  auto parser = Zeni::Rete::Parser::Create();

  parser->parse_string(network->get(), job_queue, "  ( p test-rule\r\n  ( <s> ^attr 42 ^attr 3.14159 )\r\n -(  <s> ^attr |hello world| )\r\n-->\r\n  ( write |Parser test passes.| (crlf) )\r\n)\r\n", true);

  //{
  //  std::array<std::shared_ptr<const Zeni::Rete::Symbol>, 3> symbols = {
  //    {
  //      std::make_shared<Zeni::Rete::Symbol_Variable>(Zeni::Rete::Symbol_Variable::First),
  //      std::make_shared<Zeni::Rete::Symbol_Constant_String>("attr"),
  //      std::make_shared<Zeni::Rete::Symbol_Constant_Int>(42)
  //    }
  //  };

  //  auto filter0 = Zeni::Rete::Node_Filter::Create(network->get(), job_queue, Zeni::Rete::WME(symbols[0], symbols[1], symbols[2]));
  //  auto action = Zeni::Rete::Node_Action::Create(network->get(), job_queue, "parser-substitute", false, filter0, Zeni::Rete::Variable_Indices::Create(),
  //    [](const Zeni::Rete::Node_Action &, const Zeni::Rete::Token &) {
  //    std::cout << '[' << std::flush;
  //  }, [](const Zeni::Rete::Node_Action &, const Zeni::Rete::Token &) {
  //    std::cout << ']' << std::flush;
  //  });

  //  //job_queue->give_one(
  //  //  std::make_shared<Zeni::Rete::Message_Decrement_Output_Count>(gated_passthrough1, network->get(), gated_passthrough1));
  //}

  (*network)->get_Worker_Threads()->finish_jobs();

  (*network)->insert_wme(job_queue, std::make_shared<Zeni::Rete::WME>(
    std::make_shared<Zeni::Rete::Symbol_Constant_Identifier>("S1"),
    std::make_shared<Zeni::Rete::Symbol_Constant_String>("attr"),
    std::make_shared<Zeni::Rete::Symbol_Constant_Int>(42)));

  (*network)->insert_wme(job_queue, std::make_shared<Zeni::Rete::WME>(
    std::make_shared<Zeni::Rete::Symbol_Constant_Identifier>("S1"),
    std::make_shared<Zeni::Rete::Symbol_Constant_String>("attr"),
    std::make_shared<Zeni::Rete::Symbol_Constant_Float>(3.14159)));

  (*network)->get_Worker_Threads()->finish_jobs();

  (*network)->remove_wme(job_queue, std::make_shared<Zeni::Rete::WME>(
    std::make_shared<Zeni::Rete::Symbol_Constant_Identifier>("S1"),
    std::make_shared<Zeni::Rete::Symbol_Constant_String>("attr"),
    std::make_shared<Zeni::Rete::Symbol_Constant_Int>(42)));

  (*network)->remove_wme(job_queue, std::make_shared<Zeni::Rete::WME>(
    std::make_shared<Zeni::Rete::Symbol_Constant_Identifier>("S1"),
    std::make_shared<Zeni::Rete::Symbol_Constant_String>("attr"),
    std::make_shared<Zeni::Rete::Symbol_Constant_Float>(3.14159)));

  (*network)->get_Worker_Threads()->finish_jobs();

  (*network)->excise_all(job_queue, false);

  (*network)->get_Worker_Threads()->finish_jobs();
}
