#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Concurrency/Maester.hpp"
#include "Zeni/Concurrency/Mutex.hpp"
#include "Zeni/Concurrency/Raven.hpp"
#include "Zeni/Concurrency/Thread_Pool.hpp"

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

    std::vector<std::shared_ptr<Zeni::Concurrency::Job>> jobs;
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

static void debug_dump() {
  std::cerr << std::endl;
  std::cerr << "  g_node_increments                             = " << Zeni::Rete::Counters::g_node_increments.load(std::memory_order_acquire) << std::endl;
  std::cerr << "  g_try_increment_output_counts                 = " << Zeni::Rete::Counters::g_try_increment_output_counts.load(std::memory_order_acquire) << std::endl;
  std::cerr << "  g_connect_outputs_received                    = " << Zeni::Rete::Counters::g_connect_outputs_received.load(std::memory_order_acquire) << std::endl;
  std::cerr << "  g_connect_gates_received                      = " << Zeni::Rete::Counters::g_connect_gates_received.load(std::memory_order_acquire) << std::endl;
  std::cerr << "  g_disconnect_gates_received                   = " << Zeni::Rete::Counters::g_disconnect_gates_received.load(std::memory_order_acquire) << std::endl;
  std::cerr << std::endl;
  std::cerr << "  g_disconnect_output_and_decrements_received   = " << Zeni::Rete::Counters::g_disconnect_output_and_decrements_received.load(std::memory_order_acquire) << std::endl;
  std::cerr << "  g_disconnect_output_but_nodecrements_received = " << Zeni::Rete::Counters::g_disconnect_output_but_nodecrements_received.load(std::memory_order_acquire) << std::endl;
  std::cerr << "  g_decrement_outputs_received                  = " << Zeni::Rete::Counters::g_decrement_outputs_received.load(std::memory_order_acquire) << std::endl;
  std::cerr << std::endl;
  std::cerr << "  g_tokens_inserted                             = " << Zeni::Rete::Counters::g_tokens_inserted.load(std::memory_order_acquire) << std::endl;
  std::cerr << "  g_tokens_removed                              = " << Zeni::Rete::Counters::g_tokens_removed.load(std::memory_order_acquire) << std::endl;
  std::cerr << "  g_empties_received                            = " << Zeni::Rete::Counters::g_empties_received.load(std::memory_order_acquire) << std::endl;
  std::cerr << "  g_nonempties_received                         = " << Zeni::Rete::Counters::g_nonempties_received.load(std::memory_order_acquire) << std::endl;
  std::cerr << std::endl;
  std::cerr << "  g_extra                                       ="
    << ' ' << Zeni::Rete::Counters::g_extra[0].load(std::memory_order_acquire)
    << ' ' << Zeni::Rete::Counters::g_extra[1].load(std::memory_order_acquire)
    << ' ' << Zeni::Rete::Counters::g_extra[2].load(std::memory_order_acquire)
    << ' ' << Zeni::Rete::Counters::g_extra[3].load(std::memory_order_acquire)
    << ' ' << Zeni::Rete::Counters::g_extra[4].load(std::memory_order_acquire)
    << ' ' << Zeni::Rete::Counters::g_extra[5].load(std::memory_order_acquire)
    << ' ' << Zeni::Rete::Counters::g_extra[6].load(std::memory_order_acquire)
    << ' ' << Zeni::Rete::Counters::g_extra[7].load(std::memory_order_acquire) << std::endl;
  std::cerr << std::endl;
}

static void debug_reset() {
  Zeni::Rete::Counters::g_node_increments.store(0, std::memory_order_release);
  Zeni::Rete::Counters::g_try_increment_output_counts.store(0, std::memory_order_release);
  Zeni::Rete::Counters::g_connect_gates_received.store(0, std::memory_order_release);
  Zeni::Rete::Counters::g_connect_outputs_received.store(0, std::memory_order_release);
  Zeni::Rete::Counters::g_decrement_outputs_received.store(0, std::memory_order_release);
  Zeni::Rete::Counters::g_disconnect_gates_received.store(0, std::memory_order_release);
  Zeni::Rete::Counters::g_empties_received.store(0, std::memory_order_release);
  Zeni::Rete::Counters::g_nonempties_received.store(0, std::memory_order_release);
  Zeni::Rete::Counters::g_disconnect_output_and_decrements_received.store(0, std::memory_order_release);
  Zeni::Rete::Counters::g_disconnect_output_but_nodecrements_received.store(0, std::memory_order_release);
  Zeni::Rete::Counters::g_tokens_inserted.store(0, std::memory_order_release);
  Zeni::Rete::Counters::g_tokens_removed.store(0, std::memory_order_release);
  for(int i = 0; i != 8; ++i)
    Zeni::Rete::Counters::g_extra[i].store(0, std::memory_order_release);
}

static void test_Memory_Pool();
static void test_Thread_Pool();
static void test_Rete_Network();
static void test_Parser();

int main()
{
  test_Memory_Pool();
  if (Zeni::Concurrency::Thread_Pool::get_total_workers() != 0) {
    std::cerr << "Total Workers = " << Zeni::Concurrency::Thread_Pool::get_total_workers() << std::endl;
    abort();
  }

  test_Thread_Pool();
  if (Zeni::Concurrency::Thread_Pool::get_total_workers() != 0) {
    std::cerr << "Total Workers = " << Zeni::Concurrency::Thread_Pool::get_total_workers() << std::endl;
    abort();
  }

  debug_dump();
  test_Rete_Network();
  debug_dump();
  if (Zeni::Concurrency::Thread_Pool::get_total_workers() != 0) {
    std::cerr << "Total Workers = " << Zeni::Concurrency::Thread_Pool::get_total_workers() << std::endl;
    abort();
  }
  debug_reset();

  std::cerr << "Test: ";
  for (int i = 1; i != 11; ++i) {
    std::cerr << ' ' << i;
    test_Rete_Network();
    if (Zeni::Concurrency::Thread_Pool::get_total_workers() != 0) {
      std::cerr << "Total Workers = " << Zeni::Concurrency::Thread_Pool::get_total_workers() << std::endl;
      debug_dump();
      abort();
    }
    debug_reset();
  }

  std::cerr << std::endl;

  test_Parser();
  if (Zeni::Concurrency::Thread_Pool::get_total_workers() != 0) {
    std::cerr << "Total Workers = " << Zeni::Concurrency::Thread_Pool::get_total_workers() << std::endl;
    debug_dump();
    abort();
  }

  return 0;
}

void test_Memory_Pool() {
  int * i_ptr = new int[42];
  delete[] i_ptr;
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

  std::vector<std::shared_ptr<Zeni::Concurrency::Job>> jobs;
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

void test_Rete_Network() {
  const auto network = Zeni::Rete::Network::Create(Zeni::Rete::Network::Printed_Output::None, Zeni::Concurrency::Thread_Pool::Create());

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
      auto filter0 = Zeni::Rete::Node_Filter::Create(network->get(), (*network)->get_Thread_Pool()->get_main_Job_Queue(), Zeni::Rete::WME(symbols[0], symbols[0], symbols[0]));
      //auto passthrough0 = Zeni::Rete::Node_Passthrough::Create(network->get(), (*network)->get_Thread_Pool()->get_main_Job_Queue(), filter0);
      auto filter1 = Zeni::Rete::Node_Filter::Create(network->get(), (*network)->get_Thread_Pool()->get_main_Job_Queue(), Zeni::Rete::WME(symbols[0], symbols[0], symbols[1]));
      auto unary_gate1 = Zeni::Rete::Node_Unary_Gate::Create(network->get(), (*network)->get_Thread_Pool()->get_main_Job_Queue(), filter1);
      auto gated_passthrough01 = Zeni::Rete::Node_Passthrough_Gated::Create(network->get(), (*network)->get_Thread_Pool()->get_main_Job_Queue(), filter0, unary_gate1);
      auto action = Zeni::Rete::Node_Action::Create(network->get(), (*network)->get_Thread_Pool()->get_main_Job_Queue(), "hello-world", false, gated_passthrough01, std::make_shared<Zeni::Rete::Variable_Indices>(),
        [](const Zeni::Rete::Node_Action &, const Zeni::Rete::Token &) {
        std::cout << '(';
      }, [](const Zeni::Rete::Node_Action &, const Zeni::Rete::Token &) {
        std::cout << ')';
      });

      //(*network)->get_Thread_Pool()->get_main_Job_Queue()->give_one(
      //  std::make_shared<Zeni::Rete::Raven_Decrement_Output_Count>(gated_passthrough1, network->get(), gated_passthrough1));
      //(*network)->get_Thread_Pool()->get_main_Job_Queue()->give_one(
      //  std::make_shared<Zeni::Rete::Raven_Disconnect_Output>(network->get(), network->get(), filter1, true));
    }

    //(*network)->get_Thread_Pool()->finish_jobs();

    (*network)->insert_wme((*network)->get_Thread_Pool()->get_main_Job_Queue(), std::make_shared<Zeni::Rete::WME>(symbols[0], symbols[0], symbols[0]));

    //(*network)->get_Thread_Pool()->finish_jobs();

    (*network)->insert_wme((*network)->get_Thread_Pool()->get_main_Job_Queue(), std::make_shared<Zeni::Rete::WME>(symbols[0], symbols[0], symbols[1]));

    //(*network)->get_Thread_Pool()->finish_jobs();

    (*network)->excise_rule((*network)->get_Thread_Pool()->get_main_Job_Queue(), "hello-world", false);

    //(*network)->get_Thread_Pool()->finish_jobs();

    {
      auto filter0 = Zeni::Rete::Node_Filter::Create(network->get(), (*network)->get_Thread_Pool()->get_main_Job_Queue(), Zeni::Rete::WME(symbols[0], symbols[0], symbols[0]));
      //auto passthrough0 = Zeni::Rete::Node_Passthrough::Create(network->get(), (*network)->get_Thread_Pool()->get_main_Job_Queue(), filter0);
      auto filter1 = Zeni::Rete::Node_Filter::Create(network->get(), (*network)->get_Thread_Pool()->get_main_Job_Queue(), Zeni::Rete::WME(symbols[0], symbols[0], symbols[1]));
      auto unary_gate0 = Zeni::Rete::Node_Unary_Gate::Create(network->get(), (*network)->get_Thread_Pool()->get_main_Job_Queue(), filter0);
      auto gated_passthrough10 = Zeni::Rete::Node_Passthrough_Gated::Create(network->get(), (*network)->get_Thread_Pool()->get_main_Job_Queue(), filter1, unary_gate0);
      auto action = Zeni::Rete::Node_Action::Create(network->get(), (*network)->get_Thread_Pool()->get_main_Job_Queue(), "gday-world", false, gated_passthrough10, std::make_shared<Zeni::Rete::Variable_Indices>(),
        [](const Zeni::Rete::Node_Action &, const Zeni::Rete::Token &) {
        std::cout << '[';
      }, [](const Zeni::Rete::Node_Action &, const Zeni::Rete::Token &) {
        std::cout << ']';
      });

      //(*network)->get_Thread_Pool()->get_main_Job_Queue()->give_one(
      //  std::make_shared<Zeni::Rete::Raven_Decrement_Output_Count>(gated_passthrough1, network->get(), gated_passthrough1));
    }

    //(*network)->get_Thread_Pool()->finish_jobs();

    (*network)->remove_wme((*network)->get_Thread_Pool()->get_main_Job_Queue(), std::make_shared<Zeni::Rete::WME>(symbols[0], symbols[0], symbols[0]));

    //(*network)->get_Thread_Pool()->finish_jobs();

    (*network)->remove_wme((*network)->get_Thread_Pool()->get_main_Job_Queue(), std::make_shared<Zeni::Rete::WME>(symbols[0], symbols[0], symbols[1]));

    //(*network)->get_Thread_Pool()->finish_jobs();

    //(*network)->excise_all((*network)->get_Thread_Pool()->get_main_Job_Queue());
  }
}

void test_Parser() {
  const auto network = Zeni::Rete::Network::Create();

  Zeni::Rete::Parser parser;

  parser.parse_string(network->get(), "sp {test-rule\r\n  (<s> ^attr 42)\r\n  (<s> ^attr 3.14159)\r\n-->\r\n}\r\n", true);

  (*network)->insert_wme((*network)->get_Thread_Pool()->get_main_Job_Queue(), std::make_shared<Zeni::Rete::WME>(
    std::make_shared<Zeni::Rete::Symbol_Identifier>("S1"),
    std::make_shared<Zeni::Rete::Symbol_Constant_String>("attr"),
    std::make_shared<Zeni::Rete::Symbol_Constant_Int>(42)));

  (*network)->get_Thread_Pool()->finish_jobs();

  //(*network)->remove_wme((*network)->get_Thread_Pool()->get_main_Job_Queue(), std::make_shared<Zeni::Rete::WME>(
  //  std::make_shared<Zeni::Rete::Symbol_Identifier>("S1"),
  //  std::make_shared<Zeni::Rete::Symbol_Constant_String>("attr"),
  //  std::make_shared<Zeni::Rete::Symbol_Constant_Int>(42)));

  //(*network)->get_Thread_Pool()->finish_jobs();
}
