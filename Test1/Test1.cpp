#include "Zeni/Concurrency/Container/Antiable_List.hpp"
#include "Zeni/Concurrency/Container/Ordered_List.hpp"
#include "Zeni/Concurrency/Container/Stack.hpp"
#include "Zeni/Concurrency/Container/Queue.hpp"
#include "Zeni/Concurrency/Container/Unordered_List.hpp"
#include "Zeni/Concurrency/Internal/Epoch_List.hpp"
#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Concurrency/Memory_Pools.hpp"
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
#include <iostream>
#include <list>
#include <random>
#include <string>
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

//static std::atomic_int64_t g_num_recvs = 0;

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

static void test_Worker_Threads();
static void test_Stack();
static void test_Queue();
static void test_Epoch_List();
static void test_Unordered_List();
static void test_Ordered_List();
static void test_Antiable_List();
//static void test_Deque();
static void test_Rete_Network();
static void test_Parser();

int main()
{
  test_Worker_Threads();
  if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
    std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
    abort();
  }

  for (int i = 0; i != 80; ++i) {
    test_Stack();
    if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
      std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
      abort();
    }
    std::cout << 'S' << std::flush;
  }
  std::cout << std::endl;

  for (int i = 0; i != 80; ++i) {
    test_Queue();
    if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
      std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
      abort();
    }
    std::cout << 'Q' << std::flush;
  }
  std::cout << std::endl;

  for (int i = 0; i != 80; ++i) {
    test_Epoch_List();
    if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
      std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
      abort();
    }
    std::cout << 'E' << std::flush;
  }
  std::cout << std::endl;

  for (int i = 0; i != 80; ++i) {
    test_Unordered_List();
    if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
      std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
      abort();
    }
    std::cout << 'U' << std::flush;
  }
  std::cout << std::endl;

  for (int i = 0; i != 80; ++i) {
    test_Ordered_List();
    if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
      std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
      abort();
    }
    std::cout << 'O' << std::flush;
  }
  std::cout << std::endl;

  //for (int i = 0; i != 80; ++i) {
  //  test_Antiable_List();
  //  if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
  //    std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
  //    abort();
  //  }
  //  std::cout << 'A' << std::flush;
  //}
  //std::cout << std::endl;

  //for (int i = 0; i != 100; ++i) {
  //  test_Deque();
  //  if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
  //    std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
  //    abort();
  //  }
  //  std::cout << 'D' << std::flush;
  //}
  //std::cout << std::endl;

  Zeni::Rete::Debug_Counters::print(std::cerr);
  test_Rete_Network();
  Zeni::Rete::Debug_Counters::print(std::cerr);
  if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
    std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
    abort();
  }
  Zeni::Rete::Debug_Counters::reset();

  std::cerr << "Test: ";
  for (int i = 1; i != 11; ++i) {
    std::cerr << ' ' << i;
    test_Rete_Network();
    if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
      std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
      Zeni::Rete::Debug_Counters::print(std::cerr);
      abort();
    }
    Zeni::Rete::Debug_Counters::reset();
  }

  std::cerr << std::endl;

  test_Parser();
  if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
    std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
    Zeni::Rete::Debug_Counters::print(std::cerr);
    abort();
  }

  return 0;
}

void test_Worker_Threads() {
  std::unordered_map<std::string, Gossip::Ptr> gossips;

  auto worker_threads = Zeni::Concurrency::Worker_Threads::Create();
  const auto job_queue = worker_threads->get_main_Job_Queue();

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

void test_Stack() {
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

  auto worker_threads = Zeni::Concurrency::Worker_Threads::Create();
  const auto job_queue = worker_threads->get_main_Job_Queue();

  std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
  for (int i = 0; i != 4; ++i) {
    jobs.emplace_back(std::make_shared<Pusher>(stack));
    jobs.emplace_back(std::make_shared<Popper>(stack));
  }
  job_queue->give_many(std::move(jobs));

  worker_threads->finish_jobs();

  //std::cout << std::endl;
}

void test_Queue() {
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

  auto worker_threads = Zeni::Concurrency::Worker_Threads::Create();
  const auto job_queue = worker_threads->get_main_Job_Queue();

  std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
  for (int i = 0; i != 4; ++i) {
    jobs.emplace_back(std::make_shared<Pusher>(queue));
    jobs.emplace_back(std::make_shared<Popper>(queue));
  }
  job_queue->give_many(std::move(jobs));

  worker_threads->finish_jobs();

  //std::cout << std::endl;
}

void test_Epoch_List() {
  class Epocher : public Zeni::Concurrency::Job {
  public:
    Epocher(const std::shared_ptr<Zeni::Concurrency::Epoch_List> &epoch_list) : m_epoch_list(epoch_list), dre(rd()) {}

    void execute() noexcept override {
      while (m_to_acquire + m_to_release != 0) {
        const int64_t index = std::uniform_int_distribution<int64_t>(1, std::min(m_to_acquire, m_acquire_cap - m_to_release) + m_to_release)(dre);
        if (index > m_to_release) {
          m_epochs.push_back(m_epoch_list->acquire());
          --m_to_acquire;
          ++m_to_release;
        }
        else {
          auto selected = m_epochs.begin();
          std::advance(selected, index - 1);
          [[maybe_unused]] const bool success = m_epoch_list->try_release(*selected);
          if (!success)
            std::cerr << 'X' << std::flush;
          m_epochs.erase(selected);
          --m_to_release;
        }
      }
    }

  private:
    std::shared_ptr<Zeni::Concurrency::Epoch_List> m_epoch_list;
    std::vector<uint64_t> m_epochs;
    int64_t m_to_acquire = 256;
    int64_t m_acquire_cap = 16;
    int64_t m_to_release = 0;
    std::random_device rd;
    std::default_random_engine dre;
  };

  const auto epoch_list = std::make_shared<Zeni::Concurrency::Epoch_List>();

  auto worker_threads = Zeni::Concurrency::Worker_Threads::Create();
  const auto job_queue = worker_threads->get_main_Job_Queue();

  std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
  for (int i = 0; i != 8; ++i)
    jobs.emplace_back(std::make_shared<Epocher>(epoch_list));
  job_queue->give_many(std::move(jobs));

  worker_threads->finish_jobs();

  //std::cout << std::endl;
}

void test_Unordered_List() {
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

  auto worker_threads = Zeni::Concurrency::Worker_Threads::Create();
  const auto job_queue = worker_threads->get_main_Job_Queue();

  std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
  for (int i = 0; i != 8; ++i)
    jobs.emplace_back(std::make_shared<Lister>(unordered_list));
  job_queue->give_many(std::move(jobs));

  worker_threads->finish_jobs();

  //std::cout << std::endl;
}

void test_Ordered_List() {
  class Lister : public Zeni::Concurrency::Job {
  public:
    Lister(const std::shared_ptr<Zeni::Concurrency::Ordered_List<int64_t>> &ordered_list) : m_ordered_list(ordered_list), dre(rd()) {}

    void execute() noexcept override {
      m_values_to_acquire.reserve(m_to_acquire);
      for (int i = 0; i != m_to_acquire; ++i)
        m_values_to_acquire.push_back(i);

      while (m_to_acquire + m_to_release != 0) {
        const int64_t index = std::uniform_int_distribution<int64_t>(1, std::min(m_to_acquire, m_acquire_cap - m_to_release) + m_to_release)(dre);
        if (index > m_to_release) {
          const int64_t index2 = std::uniform_int_distribution<int64_t>(0, m_to_acquire - 1)(dre);
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
    std::shared_ptr<Zeni::Concurrency::Ordered_List<int64_t>> m_ordered_list;
    std::vector<uint64_t> m_values_to_acquire;
    std::vector<uint64_t> m_values_to_release;
    int64_t m_to_acquire = 256;
    int64_t m_acquire_cap = 16;
    int64_t m_to_release = 0;
    std::random_device rd;
    std::default_random_engine dre;
  };

  const auto ordered_list = std::make_shared<Zeni::Concurrency::Ordered_List<int64_t>>();

  auto worker_threads = Zeni::Concurrency::Worker_Threads::Create();
  const auto job_queue = worker_threads->get_main_Job_Queue();

  std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
  for (int i = 0; i != 8; ++i)
    jobs.emplace_back(std::make_shared<Lister>(ordered_list));
  job_queue->give_many(std::move(jobs));

  worker_threads->finish_jobs();

  //std::cout << std::endl;
}

void test_Antiable_List() {
  class Antiable : public Zeni::Concurrency::Job {
  public:
    Antiable(const std::shared_ptr<Zeni::Concurrency::Epoch_List> &epoch_list, const std::shared_ptr<Zeni::Concurrency::Antiable_List<int64_t>> &antiable_list)
      : m_epoch_list(epoch_list),
      m_antiable_list(antiable_list),
      dre(rd())
    {
    }

    void execute() noexcept override {
      while (m_to_acquire + m_to_release != 0) {
        const int64_t index = std::uniform_int_distribution<int64_t>(1, std::min(m_to_acquire, m_acquire_cap - m_to_release) + m_to_release)(dre);
        if (index > m_to_release) {
          m_antiable_list->insert(m_epoch_list, m_to_acquire);
          m_values.push_back(m_to_acquire);
          --m_to_acquire;
          ++m_to_release;
        }
        else {
          auto selected = m_values.begin();
          std::advance(selected, index - 1);
          while (!m_antiable_list->try_erase(m_epoch_list, *selected));
          //[[maybe_unused]] const bool success = m_antiable_list->try_erase(earliest_epoch, current_epoch, *selected);
          //if (!success)
            //std::cerr << 'X' << std::flush;
          m_values.erase(selected);
          --m_to_release;
        }
      }
    }

  private:
    std::shared_ptr<Zeni::Concurrency::Epoch_List> m_epoch_list;
    std::shared_ptr<Zeni::Concurrency::Antiable_List<int64_t>> m_antiable_list;
    std::vector<uint64_t> m_values;
    int64_t m_to_acquire = 256;
    int64_t m_acquire_cap = 16;
    int64_t m_to_release = 0;
    std::random_device rd;
    std::default_random_engine dre;
  };

  const auto epoch_list = std::make_shared<Zeni::Concurrency::Epoch_List>();
  const auto antiable_list = std::make_shared<Zeni::Concurrency::Antiable_List<int64_t>>();

  auto worker_threads = Zeni::Concurrency::Worker_Threads::Create();
  const auto job_queue = worker_threads->get_main_Job_Queue();

  std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
  for (int i = 0; i != 8; ++i)
    jobs.emplace_back(std::make_shared<Antiable>(epoch_list, antiable_list));
  job_queue->give_many(std::move(jobs));

  worker_threads->finish_jobs();

  //std::cout << std::endl;
}

//void test_Deque() {
//  class Front_Pusher : public Zeni::Concurrency::Job {
//  public:
//    Front_Pusher(const std::shared_ptr<Zeni::Concurrency::Deque<int>> &deque) : m_deque(deque) {}
//
//    void execute() noexcept override {
//      for (int i = 0; i != 10000; ++i)
//        m_deque->push_front(i);
//      while (!m_deque->empty());
//    }
//
//  private:
//    std::shared_ptr<Zeni::Concurrency::Deque<int>> m_deque;
//  };
//
//  class Back_Pusher : public Zeni::Concurrency::Job {
//  public:
//    Back_Pusher(const std::shared_ptr<Zeni::Concurrency::Deque<int>> &deque) : m_deque(deque) {}
//
//    void execute() noexcept override {
//      for (int i = 0; i != 10000; ++i)
//        m_deque->push_back(i);
//      while (!m_deque->empty());
//    }
//
//  private:
//    std::shared_ptr<Zeni::Concurrency::Deque<int>> m_deque;
//  };
//
//  class Front_Popper : public Zeni::Concurrency::Job {
//  public:
//    Front_Popper(const std::shared_ptr<Zeni::Concurrency::Deque<int>> &deque) : m_deque(deque) {}
//
//    void execute() noexcept override {
//      for (int i = 0; i != 10000; ) {
//        int value;
//        if (m_deque->try_pop_front(value)) {
//          ++i;
//          //std::cout << value;
//        }
//      }
//      while (!m_deque->empty());
//    }
//
//  private:
//    std::shared_ptr<Zeni::Concurrency::Deque<int>> m_deque;
//  };
//
//  class Back_Popper : public Zeni::Concurrency::Job {
//  public:
//    Back_Popper(const std::shared_ptr<Zeni::Concurrency::Deque<int>> &deque) : m_deque(deque) {}
//
//    void execute() noexcept override {
//      for (int i = 0; i != 10000; ) {
//        int value;
//        if (m_deque->try_pop_back(value)) {
//          ++i;
//          //std::cout << value;
//        }
//      }
//      while (!m_deque->empty());
//    }
//
//  private:
//    std::shared_ptr<Zeni::Concurrency::Deque<int>> m_deque;
//  };
//
//  const auto deque = std::make_shared<Zeni::Concurrency::Deque<int>>();
//
//  auto worker_threads = Zeni::Concurrency::Worker_Threads::Create();
//  const auto job_queue = worker_threads->get_main_Job_Queue();
//
//  std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
//  for (int i = 0; i != 2; ++i) {
//    jobs.emplace_back(std::make_shared<Front_Pusher>(deque));
//    jobs.emplace_back(std::make_shared<Front_Popper>(deque));
//    jobs.emplace_back(std::make_shared<Back_Pusher>(deque));
//    jobs.emplace_back(std::make_shared<Back_Popper>(deque));
//  }
//  job_queue->give_many(std::move(jobs));
//
//  worker_threads->finish_jobs();
//
//  //std::cout << std::endl;
//}

void test_Rete_Network() {
  const auto network = Zeni::Rete::Network::Create(Zeni::Rete::Network::Printed_Output::None, Zeni::Concurrency::Worker_Threads::Create());
  const auto job_queue = (*network)->get_Worker_Threads()->get_main_Job_Queue();

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

void test_Parser() {
  const auto network = Zeni::Rete::Network::Create();
  const auto job_queue = (*network)->get_Worker_Threads()->get_main_Job_Queue();

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
