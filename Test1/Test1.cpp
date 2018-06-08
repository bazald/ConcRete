#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Concurrency/Maester.hpp"
#include "Zeni/Concurrency/Memory_Pools.hpp"
#include "Zeni/Concurrency/Mutex.hpp"
#include "Zeni/Concurrency/Raven.hpp"
#include "Zeni/Concurrency/Thread_Pool.hpp"

#include "Zeni/Rete/Internal/Debug_Counters.hpp"
#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Node_Filter.hpp"
#include "Zeni/Rete/Node_Passthrough_Gated.hpp"
#include "Zeni/Rete/Node_Unary_Gate.hpp"
#include "Zeni/Rete/Parser.hpp"
#include "Zeni/Rete/Raven_Decrement_Output_Count.hpp"
#include "Zeni/Rete/Raven_Disconnect_Output.hpp"

#include <array>
#include <iostream>
#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>

class Whisper : public Zeni::Concurrency::Raven {
public:
  typedef std::shared_ptr<Whisper> Ptr;

  Whisper(const std::shared_ptr<Zeni::Concurrency::Maester> &recipient, const std::string_view message)
    : Raven(recipient), m_message(message)
  {
  }

  std::string get_message() const {
    return m_message;
  }

private:
  std::string m_message;
};

//static std::atomic_int64_t g_num_recvs = 0;

class Gossip : public Zeni::Concurrency::Maester {
public:
  typedef std::shared_ptr<Gossip> Ptr;

  void receive(const std::shared_ptr<const Zeni::Concurrency::Raven> raven) noexcept override {
    const auto whisper = std::dynamic_pointer_cast<const Whisper>(raven);

    std::cerr << whisper->get_message() + "\n";
    //++g_num_recvs;

    std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
    Zeni::Concurrency::Mutex::Lock mutex_lock(m_mutex);
    for (Ptr gossip : m_gossips)
      jobs.emplace_back(std::make_shared<Whisper>(gossip, whisper->get_message()));

    const auto job_queue = raven->get_Job_Queue();
    job_queue->give_many(std::move(jobs));
  }

  void tell(const Ptr gossip) {
    Zeni::Concurrency::Mutex::Lock mutex_lock(m_mutex);
    m_gossips.emplace(gossip);
  }

private:
  std::unordered_set<Ptr> m_gossips;
};

static void test_Thread_Pool();
static void test_Memory_Pool();
static void test_Rete_Network();
static void test_Parser();

int main()
{
  test_Thread_Pool();
  if (Zeni::Concurrency::Thread_Pool::get_total_workers() != 0) {
    std::cerr << "Total Workers = " << Zeni::Concurrency::Thread_Pool::get_total_workers() << std::endl;
    abort();
  }

  test_Memory_Pool();
  if (Zeni::Concurrency::Thread_Pool::get_total_workers() != 0) {
    std::cerr << "Total Workers = " << Zeni::Concurrency::Thread_Pool::get_total_workers() << std::endl;
    abort();
  }

  Zeni::Rete::Debug_Counters::print(std::cerr);
  test_Rete_Network();
  Zeni::Rete::Debug_Counters::print(std::cerr);
  if (Zeni::Concurrency::Thread_Pool::get_total_workers() != 0) {
    std::cerr << "Total Workers = " << Zeni::Concurrency::Thread_Pool::get_total_workers() << std::endl;
    abort();
  }
  Zeni::Rete::Debug_Counters::reset();

  std::cerr << "Test: ";
  for (int i = 1; i != 11; ++i) {
    std::cerr << ' ' << i;
    test_Rete_Network();
    if (Zeni::Concurrency::Thread_Pool::get_total_workers() != 0) {
      std::cerr << "Total Workers = " << Zeni::Concurrency::Thread_Pool::get_total_workers() << std::endl;
      Zeni::Rete::Debug_Counters::print(std::cerr);
      abort();
    }
    Zeni::Rete::Debug_Counters::reset();
  }

  std::cerr << std::endl;

  test_Parser();
  if (Zeni::Concurrency::Thread_Pool::get_total_workers() != 0) {
    std::cerr << "Total Workers = " << Zeni::Concurrency::Thread_Pool::get_total_workers() << std::endl;
    Zeni::Rete::Debug_Counters::print(std::cerr);
    abort();
  }

  return 0;
}

void test_Thread_Pool() {
  std::unordered_map<std::string, Gossip::Ptr> gossips;

  auto thread_pool = Zeni::Concurrency::Thread_Pool::Create();

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

  thread_pool->get_main_Job_Queue()->give_many(std::move(jobs));
  jobs.clear();

  //for(int i = 0; i != 1000000; ++i)
  //  thread_pool.get_queue()->give(std::make_shared<Whisper>(gossips["alice"], "Meh."));

  thread_pool->finish_jobs();

  for (std::string message : {"I get it.", "That was a bad one.", "I'm sorry. :-("})
    jobs.emplace_back(std::make_shared<Whisper>(gossips["alice"], message));
  thread_pool->get_main_Job_Queue()->give_many(std::move(jobs));

  //std::cout << "g_num_recvs == " << g_num_recvs << std::endl;
}

static std::atomic_int64_t g_memory_test_complete = false;

void test_Memory_Pool() {
  class Pool_Clearer : public Zeni::Concurrency::Job {
  public:
    void execute() noexcept override {
      do {
        Zeni::Concurrency::Memory_Pools::clear_pools();
      } while (g_memory_test_complete.load(std::memory_order_acquire) == false);
    }
  };

  class List_Runner : public Zeni::Concurrency::Job {
  public:
    void execute() noexcept override {
      std::list<int> numbers;
      for (int i = 0; i != 1000; ++i)
        numbers.push_back(i);
      while (!numbers.empty())
        numbers.pop_back();
    }
  };

  class Thread_Runner : public Zeni::Concurrency::Job {
  public:
    void execute() noexcept override {
      {
        auto thread_pool = Zeni::Concurrency::Thread_Pool::Create(3);

        std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
        for (int i = 0; i != 100; ++i)
          jobs.push_back(std::make_shared<List_Runner>());
        thread_pool->get_main_Job_Queue()->give_many(jobs);

        thread_pool->finish_jobs();
      }

      g_memory_test_complete.store(true, std::memory_order_release);
    }
  };

  auto thread_pool = Zeni::Concurrency::Thread_Pool::Create(2);

  std::vector<std::shared_ptr<Zeni::Concurrency::IJob>> jobs;
  jobs.push_back(std::make_shared<Pool_Clearer>());
  jobs.push_back(std::make_shared<Thread_Runner>());
  thread_pool->get_main_Job_Queue()->give_many(jobs);

  thread_pool->finish_jobs();
}

void test_Rete_Network() {
  const auto network = Zeni::Rete::Network::Create(Zeni::Rete::Network::Printed_Output::None, Zeni::Concurrency::Thread_Pool::Create());
  const auto job_queue = (*network)->get_Thread_Pool()->get_main_Job_Queue();

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
      //  std::make_shared<Zeni::Rete::Raven_Decrement_Output_Count>(gated_passthrough1, network->get(), gated_passthrough1));
      //job_queue->give_one(
      //  std::make_shared<Zeni::Rete::Raven_Disconnect_Output>(network->get(), network->get(), filter1, true));
    }

    //(*network)->get_Thread_Pool()->finish_jobs();

    (*network)->insert_wme(job_queue, std::make_shared<Zeni::Rete::WME>(symbols[0], symbols[0], symbols[0]));

    //(*network)->get_Thread_Pool()->finish_jobs();

    (*network)->insert_wme(job_queue, std::make_shared<Zeni::Rete::WME>(symbols[0], symbols[0], symbols[1]));

    //(*network)->get_Thread_Pool()->finish_jobs();

    (*network)->excise_rule(job_queue, "hello-world", false);

    //(*network)->get_Thread_Pool()->finish_jobs();

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
      //  std::make_shared<Zeni::Rete::Raven_Decrement_Output_Count>(gated_passthrough1, network->get(), gated_passthrough1));
    }

    //(*network)->get_Thread_Pool()->finish_jobs();

    (*network)->remove_wme(job_queue, std::make_shared<Zeni::Rete::WME>(symbols[0], symbols[0], symbols[0]));

    //(*network)->get_Thread_Pool()->finish_jobs();

    (*network)->remove_wme(job_queue, std::make_shared<Zeni::Rete::WME>(symbols[0], symbols[0], symbols[1]));

    //(*network)->get_Thread_Pool()->finish_jobs();

    //(*network)->excise_all(job_queue);
  }
}

void test_Parser() {
  const auto network = Zeni::Rete::Network::Create();
  const auto job_queue = (*network)->get_Thread_Pool()->get_main_Job_Queue();

  auto parser = Zeni::Rete::Parser::Create();

  parser->parse_string(network->get(), job_queue, "sp {test-rule\r\n  (<s> ^attr 42)\r\n  (<s> ^attr 3.14159)\r\n-->\r\n}\r\n", true);

  (*network)->insert_wme(job_queue, std::make_shared<Zeni::Rete::WME>(
    std::make_shared<Zeni::Rete::Symbol_Identifier>("S1"),
    std::make_shared<Zeni::Rete::Symbol_Constant_String>("attr"),
    std::make_shared<Zeni::Rete::Symbol_Constant_Int>(42)));

  (*network)->get_Thread_Pool()->finish_jobs();

  //(*network)->remove_wme(job_queue, std::make_shared<Zeni::Rete::WME>(
  //  std::make_shared<Zeni::Rete::Symbol_Identifier>("S1"),
  //  std::make_shared<Zeni::Rete::Symbol_Constant_String>("attr"),
  //  std::make_shared<Zeni::Rete::Symbol_Constant_Int>(42)));

  //(*network)->get_Thread_Pool()->finish_jobs();
}
