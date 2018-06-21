#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Concurrency/Memory_Pools.hpp"
#include "Zeni/Concurrency/Message.hpp"
#include "Zeni/Concurrency/Mutex.hpp"
#include "Zeni/Concurrency/Recipient.hpp"
#include "Zeni/Concurrency/Stack.hpp"
#include "Zeni/Concurrency/Queue.hpp"
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
//static void test_Memory_Pool();
static void test_Rete_Network();
static void test_Parser();

int main()
{
  test_Worker_Threads();
  if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
    std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
    abort();
  }

  for (int i = 0; i != 100; ++i) {
    test_Stack();
    std::cout << 'S';
  }
  std::cout << std::endl;
  if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
    std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
    abort();
  }

  for (int i = 0; i != 100; ++i) {
    test_Queue();
    std::cout << 'Q';
  }
  std::cout << std::endl;
  if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
    std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
    abort();
  }

  //test_Memory_Pool();
  //if (Zeni::Concurrency::Worker_Threads::get_total_workers() != 0) {
  //  std::cerr << "Total Workers = " << Zeni::Concurrency::Worker_Threads::get_total_workers() << std::endl;
  //  abort();
  //}

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
      for (int i = 0; i != 100; ++i)
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
      for (int i = 0; i != 100; ) {
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
      for (int i = 0; i != 100; ++i)
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
      for (int i = 0; i != 100; ) {
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

//static std::atomic_int64_t g_memory_test_complete = false;
//
//void test_Memory_Pool() {
//  class Pool_Clearer : public Zeni::Concurrency::Job {
//  public:
//    void execute() noexcept override {
//      do {
//        Zeni::Concurrency::Memory_Pools::clear_pools();
//      } while (g_memory_test_complete.load(std::memory_order_acquire) == false);
//    }
//  };
//
//  class List_Runner : public Zeni::Concurrency::Job {
//  public:
//    void execute() noexcept override {
//      std::list<int> numbers;
//      for (int i = 0; i != 1000; ++i)
//        numbers.push_back(i);
//      while (!numbers.empty())
//        numbers.pop_back();
//    }
//  };
//
//  class Thread_Runner : public Zeni::Concurrency::Job {
//  public:
//    void execute() noexcept override {
//      {
//        auto worker_threads = Zeni::Concurrency::Worker_Threads::Create(3);
//
//        std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
//        for (int i = 0; i != 100; ++i)
//          jobs.push_back(std::make_shared<List_Runner>());
//        worker_threads->get_main_Job_Queue()->give_many(jobs);
//
//        worker_threads->finish_jobs();
//      }
//
//      g_memory_test_complete.store(true, std::memory_order_release);
//    }
//  };
//
//  auto worker_threads = Zeni::Concurrency::Worker_Threads::Create(2);
//
//  std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
//  jobs.push_back(std::make_shared<Pool_Clearer>());
//  jobs.push_back(std::make_shared<Thread_Runner>());
//  worker_threads->get_main_Job_Queue()->give_many(jobs);
//
//  worker_threads->finish_jobs();
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
        std::cout << '(';
      }, [](const Zeni::Rete::Node_Action &, const Zeni::Rete::Token &) {
        std::cout << ')';
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
        std::cout << '[';
      }, [](const Zeni::Rete::Node_Action &, const Zeni::Rete::Token &) {
        std::cout << ']';
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
