#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Concurrency/Maester.hpp"
#include "Zeni/Concurrency/Memory_Pool.hpp"
#include "Zeni/Concurrency/Mutex.hpp"
#include "Zeni/Concurrency/Raven.hpp"
#include "Zeni/Concurrency/Thread_Pool.hpp"

#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Node_Filter.hpp"
#include "Zeni/Rete/Node_Passthrough_Gated.hpp"
#include "Zeni/Rete/Node_Unary_Gate.hpp"
#include "Zeni/Rete/Parser.hpp"
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

  void receive(Zeni::Concurrency::Job_Queue &job_queue, const std::shared_ptr<const Zeni::Concurrency::Raven> raven) noexcept override {
    const auto whisper = std::dynamic_pointer_cast<const Whisper>(raven);

    std::cerr << whisper->get_message() + "\n";
    //++g_num_recvs;

    std::vector<std::shared_ptr<Zeni::Concurrency::Job>> jobs;
    Zeni::Concurrency::Mutex::Lock mutex_lock(m_mutex);
    for (Ptr gossip : m_gossips)
      jobs.emplace_back(std::make_shared<Whisper>(gossip, whisper->get_message()));

    job_queue.give_many(std::move(jobs));
  }

  void tell(const Ptr gossip) {
    Zeni::Concurrency::Mutex::Lock mutex_lock(m_mutex);
    m_gossips.emplace(gossip);
  }

private:
  std::unordered_set<Ptr> m_gossips;
};

static void test_Memory_Pool();
static void test_Thread_Pool();
static void test_Rete_Network();
static void test_Parser();

int main()
{
  test_Memory_Pool();
  test_Thread_Pool();
  test_Rete_Network();
  test_Parser();

  Zeni::Concurrency::Memory_Pool::get().clear();

  return 0;
}

void test_Memory_Pool() {
  int * i_ptr = new int[42];
  delete[] i_ptr;
}

void test_Thread_Pool() {
  std::unordered_map<std::string, Gossip::Ptr> gossips;

  Zeni::Concurrency::Thread_Pool thread_pool;

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

  thread_pool.get_Job_Queue()->give_many(std::move(jobs));
  jobs.clear();

  //for(int i = 0; i != 1000000; ++i)
  //  thread_pool.get_queue()->give(std::make_shared<Whisper>(gossips["alice"], "Meh."));

  thread_pool.get_Job_Queue()->wait_for_completion();

  for (std::string message : {"I get it.", "That was a bad one.", "I'm sorry. :-("})
    jobs.emplace_back(std::make_shared<Whisper>(gossips["alice"], message));
  thread_pool.get_Job_Queue()->give_many(std::move(jobs));

  //std::cout << "g_num_recvs == " << g_num_recvs << std::endl;
}

void test_Rete_Network() {
  const auto network = Zeni::Rete::Network::Create();

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
    auto filter1 = Zeni::Rete::Node_Filter::Create(network->get(), Zeni::Rete::WME(symbols[0], symbols[0], symbols[0]));
    //auto passthrough1 = Zeni::Rete::Node_Passthrough::Create(network->get(), filter1);
    auto filter2 = Zeni::Rete::Node_Filter::Create(network->get(), Zeni::Rete::WME(symbols[0], symbols[0], symbols[1]));
    auto unary_gate1 = Zeni::Rete::Node_Unary_Gate::Create(network->get(), filter2);
    auto gated_passthrough1 = Zeni::Rete::Node_Passthrough_Gated::Create(network->get(), filter1, unary_gate1);
    auto action = Zeni::Rete::Node_Action::Create(network->get(), "hello-world", false, gated_passthrough1, std::make_shared<Zeni::Rete::Variable_Indices>(),
      [](const Zeni::Rete::Node_Action &, const Zeni::Rete::Token &) {
      std::cout << "Hello world!" << std::endl;
    }, [](const Zeni::Rete::Node_Action &, const Zeni::Rete::Token &) {
      std::cout << "Goodbye world!" << std::endl;
    });
  }

  (*network)->get_Job_Queue()->wait_for_completion();

  (*network)->insert_wme(std::make_shared<Zeni::Rete::WME>(symbols[0], symbols[0], symbols[0]));

  (*network)->get_Job_Queue()->wait_for_completion();

  (*network)->insert_wme(std::make_shared<Zeni::Rete::WME>(symbols[0], symbols[0], symbols[1]));

  (*network)->get_Job_Queue()->wait_for_completion();

  (*network)->remove_wme(std::make_shared<Zeni::Rete::WME>(symbols[0], symbols[0], symbols[0]));

  (*network)->get_Job_Queue()->wait_for_completion();
}

void test_Parser() {
  const auto network = Zeni::Rete::Network::Create();

  Zeni::Rete::Parser parser;

  parser.parse_string(network->get(), "sp {test-rule\r\n  (<s> ^attr 42)\r\n  (<s> ^attr 3.14159)\r\n-->\r\n}\r\n", true);

  (*network)->insert_wme(std::make_shared<Zeni::Rete::WME>(
    std::make_shared<Zeni::Rete::Symbol_Identifier>("S1"),
    std::make_shared<Zeni::Rete::Symbol_Constant_String>("attr"),
    std::make_shared<Zeni::Rete::Symbol_Constant_Int>(42)));

  (*network)->get_Job_Queue()->wait_for_completion();

  //(*network)->remove_wme(std::make_shared<Zeni::Rete::WME>(
  //  std::make_shared<Zeni::Rete::Symbol_Identifier>("S1"),
  //  std::make_shared<Zeni::Rete::Symbol_Constant_String>("attr"),
  //  std::make_shared<Zeni::Rete::Symbol_Constant_Int>(42)));

  //(*network)->get_Job_Queue()->wait_for_completion();
}
