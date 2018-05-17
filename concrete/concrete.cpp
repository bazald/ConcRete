#include "Concurrency/maester.h"
#include "Concurrency/raven.h"
#include "Concurrency/thread_pool.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

class Whisper : public Concurrency::Raven {
public:
  typedef std::shared_ptr<Whisper> Ptr;

  Whisper(const std::shared_ptr<Concurrency::Maester> &recipient, const std::string &message)
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

class Gossip : public Concurrency::Maester {
public:
  typedef std::shared_ptr<Gossip> Ptr;

  Gossip(const Concurrency::Job_Queue::Ptr &job_queue)
    : Concurrency::Maester(job_queue)
  {
  }

  void receive(const std::shared_ptr<Concurrency::Raven> &raven) override {
    Whisper &whisper = dynamic_cast<Whisper &>(*raven);

    std::cerr << whisper.get_message() + "\n";
    //++g_num_recvs;

    std::unique_lock<std::mutex> mutex_lock(m_mutex);
    for(Ptr gossip : m_gossips)
      get_job_queue()->give(std::make_shared<Whisper>(gossip, whisper.get_message()));
  }

  void tell(const Ptr &gossip) {
    std::unique_lock<std::mutex> mutex_lock(m_mutex);
    m_gossips.insert(gossip);
  }

private:
  std::unordered_set<Ptr> m_gossips;
};

int main()
{
  {
    std::unordered_map<std::string, Gossip::Ptr> gossips;

    static const size_t num_workers = 8;
    Concurrency::Job_Queue::Ptr job_queue = std::make_shared<Concurrency::Job_Queue>(num_workers);
    Concurrency::Thread_Pool thread_pool(job_queue);

    for(std::string name : {"alice", "betty", "carol", "diane", "ellen", "farah", "gabby", "helen", "irene", "janae", "kelly"})
      gossips[name] = std::make_shared<Gossip>(job_queue);

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

    for(std::string message : {"Hi.", "Want to hear a joke?", "Why did the chicken cross the road?", "To get to the other side!"})
      job_queue->give(std::make_shared<Whisper>(gossips["alice"], message));
    //for(int i = 0; i != 1000000; ++i)
    //  job_queue->give(std::make_shared<Whisper>(gossips["alice"], "Meh."));

    job_queue->wait_for_completion();

    for(std::string message : {"I get it.", "That was a bad one.", "I'm sorry. :-("})
      job_queue->give(std::make_shared<Whisper>(gossips["alice"], message));
  }

  //std::cout << "g_num_recvs == " << g_num_recvs << std::endl;

  return 0;
}
