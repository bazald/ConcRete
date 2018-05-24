#include "Zeni/Concurrency/Maester.hpp"
#include "Zeni/Concurrency/Mutex.hpp"
#include "Zeni/Concurrency/Raven.hpp"
#include "Zeni/Concurrency/Thread_Pool.hpp"

#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Node_Filter.hpp"

#include <array>
#include <iostream>
#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>

class Whisper : public Zeni::Concurrency::Raven {
public:
  typedef std::shared_ptr<Whisper> Ptr;

  Whisper(const std::shared_ptr<Zeni::Concurrency::Maester> &recipient, const std::string &message)
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

  void receive(Zeni::Concurrency::Job_Queue &job_queue, const Zeni::Concurrency::Raven &raven) override {
    const Whisper &whisper = dynamic_cast<const Whisper &>(raven);

    std::cerr << whisper.get_message() + "\n";
    //++g_num_recvs;

    std::list<std::shared_ptr<Whisper>> whispers;
    Zeni::Concurrency::Mutex::Lock mutex_lock(m_mutex);
    for (Ptr gossip : m_gossips)
      whispers.push_back(std::make_shared<Whisper>(gossip, whisper.get_message()));

    job_queue.give_many(whispers);
  }

  void tell(const Ptr &gossip) {
    Zeni::Concurrency::Mutex::Lock mutex_lock(m_mutex);
    m_gossips.insert(gossip);
  }

private:
  std::unordered_set<Ptr> m_gossips;
};

static void test_Thread_Pool();
static void test_Rete_Network();

int main()
{
  test_Thread_Pool();
  test_Rete_Network();

  return 0;
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

  std::list<std::shared_ptr<Whisper>> whispers;
  for (std::string message : {"Hi.", "Want to hear a joke?", "Why did the chicken cross the road?", "To get to the other side!"})
    whispers.push_back(std::make_shared<Whisper>(gossips["alice"], message));

  thread_pool.get_Job_Queue()->give_many(whispers);
  whispers.clear();

  //for(int i = 0; i != 1000000; ++i)
  //  thread_pool.get_queue()->give(std::make_shared<Whisper>(gossips["alice"], "Meh."));

  thread_pool.get_Job_Queue()->wait_for_completion();

  for (std::string message : {"I get it.", "That was a bad one.", "I'm sorry. :-("})
    whispers.push_back(std::make_shared<Whisper>(gossips["alice"], message));
  thread_pool.get_Job_Queue()->give_many(whispers);

  //std::cout << "g_num_recvs == " << g_num_recvs << std::endl;
}

void test_Rete_Network() {
  const auto network =
    Zeni::Rete::Network::Create(Zeni::Rete::Network::Printed_Output::Normal);

  std::array<std::shared_ptr<const Zeni::Rete::Symbol>, 5> symbols = {
    {
      std::make_shared<Zeni::Rete::Symbol_Constant_Int>(1),
      std::make_shared<Zeni::Rete::Symbol_Constant_Int>(2),
      std::make_shared<Zeni::Rete::Symbol_Constant_Int>(3),
      std::make_shared<Zeni::Rete::Symbol_Constant_Int>(4),
      std::make_shared<Zeni::Rete::Symbol_Constant_Int>(5)
    }
  };

  auto filter = Zeni::Rete::Node_Filter::Create_Or_Increment_Output_Count(network->get(), Zeni::Rete::WME(symbols[0], symbols[0], symbols[0]));
  auto action = Zeni::Rete::Node_Action::Create_Or_Increment_Output_Count(network->get(), "hello-world", false, filter, std::make_shared<Zeni::Rete::Variable_Indices>(),
    [](const Zeni::Rete::Node_Action &rete_action, const Zeni::Rete::Token &token) {
    std::cout << "Hello world!" << std::endl;
  }, [](const Zeni::Rete::Node_Action &rete_action, const Zeni::Rete::Token &token) {
    std::cout << "Goodbye world!" << std::endl;
  });

  (*network)->insert_wme(std::make_shared<Zeni::Rete::WME>(symbols[0], symbols[0], symbols[0]));

  (*network)->get_Job_Queue()->wait_for_completion();
}
