#include "Zeni/Rete/Network.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Concurrency/Worker_Threads.hpp"
#include "Zeni/Rete/Internal/Debug_Counters.hpp"
#include "Zeni/Rete/Message_Connect_Output.hpp"
#include "Zeni/Rete/Message_Decrement_Output_Count.hpp"
#include "Zeni/Rete/Message_Disconnect_Output.hpp"
#include "Zeni/Rete/Message_Token_Insert.hpp"
#include "Zeni/Rete/Message_Token_Remove.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Node_Filter.hpp"
#include "Zeni/Rete/Token_Alpha.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <string_view>

namespace Zeni::Rete {

  class Unlocked_Network_Data {
    Unlocked_Network_Data(const Unlocked_Network_Data &) = delete;
    Unlocked_Network_Data & operator=(const Unlocked_Network_Data &) = delete;

    friend class Locked_Network_Data_Const;
    friend class Locked_Network_Data;

  public:
    struct WMEs {
      std::unordered_multimap<std::shared_ptr<const WME>, std::shared_ptr<const Token>, hash_deref<WME>, compare_deref_eq> positive;
      std::unordered_multiset<std::shared_ptr<const WME>, hash_deref<WME>, compare_deref_eq> negative;
    };

    Unlocked_Network_Data() {}

  private:
    std::unordered_map<std::string, std::shared_ptr<Node_Action>> m_rules;
    int64_t m_rule_name_index = 0;
    WMEs m_wmes;
  };

  class Locked_Network_Data;

  class Locked_Network_Data_Const {
    Locked_Network_Data_Const(const Locked_Network_Data_Const &) = delete;
    Locked_Network_Data_Const & operator=(const Locked_Network_Data_Const &) = delete;

    friend Locked_Network_Data;

  public:
    Locked_Network_Data_Const(const Network * network, const Node::Locked_Node_Data_Const &)
      : m_data(network->m_unlocked_network_data)
    {
    }

    const std::unordered_map<std::string, std::shared_ptr<Node_Action>> & get_rules() const {
      return m_data->m_rules;
    }

    int64_t get_rule_name_index() const {
      return m_data->m_rule_name_index;
    }

    const Unlocked_Network_Data::WMEs & get_wmes() const {
      return m_data->m_wmes;
    }

  private:
    const std::shared_ptr<const Unlocked_Network_Data> m_data;
  };

  class Locked_Network_Data : public Locked_Network_Data_Const {
    Locked_Network_Data(const Locked_Network_Data &) = delete;
    Locked_Network_Data & operator=(const Locked_Network_Data &) = delete;

  public:
    Locked_Network_Data(Network * network, const Node::Locked_Node_Data_Const &node_data)
      : Locked_Network_Data_Const(network, node_data),
      m_data(network->m_unlocked_network_data)
    {
    }

    std::unordered_map<std::string, std::shared_ptr<Node_Action>> & modify_rules() {
      return m_data->m_rules;
    }

    int64_t & modify_rule_name_index() {
      return m_data->m_rule_name_index;
    }

    Unlocked_Network_Data::WMEs & modify_wmes() const {
      return m_data->m_wmes;
    }

  private:
    const std::shared_ptr<Unlocked_Network_Data> m_data;
  };

  std::shared_ptr<const Network> Network::shared_from_this() const {
    return std::static_pointer_cast<const Network>(Concurrency::Recipient::shared_from_this());
  }

  std::shared_ptr<Network> Network::shared_from_this() {
    return std::static_pointer_cast<Network>(Concurrency::Recipient::shared_from_this());
  }

  Network::Instantiation::Instantiation(const std::shared_ptr<Network> network)
    : m_network(network)
  {
  }

  std::shared_ptr<Network::Instantiation> Network::Instantiation::Create(const std::shared_ptr<Network> network) {
    class Friendly_Network_Instantiation : public Instantiation {
    public:
      Friendly_Network_Instantiation(const std::shared_ptr<Network> network)
        : Instantiation(network)
      {
      }
    };

    return std::make_shared<Friendly_Network_Instantiation>(network);
  }

  Network::Instantiation::~Instantiation() {
    m_network->Destroy();
  }

  std::shared_ptr<const Network> Network::Instantiation::get() const {
    return m_network;
  }

  std::shared_ptr<Network> Network::Instantiation::get() {
    return m_network;
  }

  const Network * Network::Instantiation::operator*() const {
    return m_network.get();
  }

  Network * Network::Instantiation::operator*() {
    return m_network.get();
  }

  const Network * Network::Instantiation::operator->() const {
    return m_network.get();
  }

  Network * Network::Instantiation::operator->() {
    return m_network.get();
  }

  Network::Network(const Network::Printed_Output printed_output)
    : Node(0, 0, 1),
    m_worker_threads(Concurrency::Worker_Threads::Create()),
    m_unlocked_network_data(std::make_shared<Unlocked_Network_Data>()),
    m_printed_output(printed_output)
  {
    DEBUG_COUNTER_DECREMENT(g_node_increments, 1);
  }

  std::shared_ptr<Network::Instantiation> Network::Create(const Network::Printed_Output printed_output) {
    class Friendly_Network : public Network {
    public:
      Friendly_Network(const Network::Printed_Output &printed_output)
        : Network(printed_output)
      {
      }
    };

    auto network_instantiation = Instantiation::Create(std::make_shared<Friendly_Network>(printed_output));

    return network_instantiation;
  }

  Network::Network(const Printed_Output printed_output, const std::shared_ptr<Concurrency::Worker_Threads> &worker_threads)
    : Node(0, 0, 1),
    m_worker_threads(worker_threads),
    m_unlocked_network_data(std::make_shared<Unlocked_Network_Data>()),
    m_printed_output(printed_output)
  {
    DEBUG_COUNTER_DECREMENT(g_node_increments, 1);
  }

  std::shared_ptr<Network::Instantiation> Network::Create(const Network::Printed_Output printed_output, const std::shared_ptr<Concurrency::Worker_Threads> worker_threads) {
    class Friendly_Network : public Network {
    public:
      Friendly_Network(const Network::Printed_Output &printed_output, const std::shared_ptr<Concurrency::Worker_Threads> &worker_threads)
        : Network(printed_output, worker_threads)
      {
      }
    };

    return Instantiation::Create(std::make_shared<Friendly_Network>(printed_output, worker_threads));
  }

  void Network::Destroy() {
    excise_all(m_worker_threads->get_main_Job_Queue());
    m_worker_threads->finish_jobs();
  }

  void Network::send_disconnect_from_parents(const std::shared_ptr<Network>, const std::shared_ptr<Concurrency::Job_Queue>)
  {
  }

  Network::~Network()
  {
  }

  std::shared_ptr<Concurrency::Worker_Threads> Network::get_Worker_Threads() const {
    return m_worker_threads;
  }

  std::shared_ptr<Node_Action> Network::get_rule(const std::string &name) const {
    Locked_Node_Data_Const locked_node_data(this);
    Locked_Network_Data_Const locked_network_data(this, locked_node_data);

    const auto found = locked_network_data.get_rules().find(name);
    if (found != locked_network_data.get_rules().cend())
      return found->second;

    return nullptr;
  }

  std::set<std::string> Network::get_rule_names() const {
    Locked_Node_Data_Const locked_node_data(this);
    Locked_Network_Data_Const locked_network_data(this, locked_node_data);

    std::set<std::string> rv;
    for (auto rule : locked_network_data.get_rules())
      rv.emplace(rule.first);
    return rv;
  }

  int64_t Network::get_rule_name_index() const {
    Locked_Node_Data_Const locked_node_data(this);
    Locked_Network_Data_Const locked_network_data(this, locked_node_data);

    return locked_network_data.get_rule_name_index();
  }

  void Network::set_rule_name_index(const int64_t rule_name_index_) {
    Locked_Node_Data locked_node_data(this);
    Locked_Network_Data locked_network_data(this, locked_node_data);

    locked_network_data.modify_rule_name_index() = rule_name_index_;
  }

  Network::Node_Sharing Network::get_Node_Sharing() const {
    return m_node_sharing;
  }

  Network::Printed_Output Network::get_Printed_Output() const {
    return m_printed_output;
  }

  void Network::excise_all(const std::shared_ptr<Concurrency::Job_Queue> job_queue) {
    const auto sft = shared_from_this();
    std::unordered_map<std::string, std::shared_ptr<Node_Action>> rules;

    {
      Locked_Node_Data locked_node_data(this);
      Locked_Network_Data locked_network_data(this, locked_node_data);

      rules.swap(locked_network_data.modify_rules());
    }

    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;
    jobs.reserve(rules.size());
    for (auto rule : rules)
      jobs.emplace_back(std::make_shared<Message_Disconnect_Output>(rule.second->get_input(), sft, rule.second, true));

    job_queue->give_many(std::move(jobs));
  }

  std::pair<std::shared_ptr<Node>, std::shared_ptr<Node>> Network::get_inputs() {
    return std::make_pair(nullptr, nullptr);
  }

  void Network::receive(const Message_Status_Empty &) {
    abort();
  }

  void Network::receive(const Message_Status_Nonempty &) {
    abort();
  }

  void Network::receive(const Message_Token_Insert &) {
    abort();
  }

  void Network::receive(const Message_Token_Remove &) {
    abort();
  }

  void Network::excise_rule(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::string &name, const bool user_command) {
    std::shared_ptr<Node_Action> action;

    {
      Locked_Node_Data locked_node_data(this);
      Locked_Network_Data locked_network_data(this, locked_node_data);

      auto found = locked_network_data.get_rules().find(name);
      if (found == locked_network_data.get_rules().end()) {
        //#ifndef NDEBUG
        //      std::cerr << "Rule '" << name << "' not found." << std::endl;
        //#endif
      }
      else {
        //#ifndef NDEBUG
        //      std::cerr << "Rule '" << name << "' excised." << std::endl;
        //#endif
        action = found->second;
        locked_network_data.modify_rules().erase(found);
        if (user_command)
          std::cerr << '#';
      }
    }

    if(action)
      job_queue->give_one(std::make_shared<Message_Decrement_Output_Count>(action, shared_from_this(), action));
  }

  std::string Network::next_rule_name(const std::string_view prefix) {
    Locked_Node_Data locked_node_data(this);
    Locked_Network_Data locked_network_data(this, locked_node_data);

    std::ostringstream oss;
    do {
      oss.str("");
      oss << prefix << ++locked_network_data.modify_rule_name_index();
    } while (locked_network_data.get_rules().find(oss.str()) != locked_network_data.get_rules().end());
    return oss.str();
  }

  std::shared_ptr<Node_Action> Network::unname_rule(const std::string &name, const bool user_command) {
    Locked_Node_Data locked_node_data(this);
    Locked_Network_Data locked_network_data(this, locked_node_data);

    std::shared_ptr<Node_Action> ptr;
    auto found = locked_network_data.get_rules().find(name);
    if (found != locked_network_data.get_rules().end()) {
      ptr = found->second;
      locked_network_data.modify_rules().erase(found);
      if (user_command)
        std::cerr << '#';
    }
    return ptr;
  }

  void Network::insert_wme(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const WME> wme) {
    const auto sft = shared_from_this();
    const auto output_token = std::make_shared<Token_Alpha>(wme);
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

#ifdef DEBUG_OUTPUT
    std::cerr << "rete.insert" << *wme << std::endl;
#endif

    {
      Locked_Node_Data locked_node_data(this);
      Locked_Network_Data locked_network_data(this, locked_node_data);

      const Outputs &outputs = locked_node_data.get_outputs();
      Unlocked_Network_Data::WMEs &wmes = locked_network_data.modify_wmes();

      auto found = wmes.negative.find(wme);
      if (found != wmes.negative.end()) {
        wmes.negative.erase(found);
        return;
      }

      wmes.positive.emplace(wme, output_token);
      locked_node_data.modify_output_tokens().emplace(output_token);
      jobs.reserve(outputs.positive.size());
      for (auto &output : outputs.positive)
        jobs.emplace_back(std::make_shared<Message_Token_Insert>(output, sft, nullptr, output_token));
    }

    job_queue->give_many(std::move(jobs));
  }

  void Network::remove_wme(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const WME> wme) {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

#ifdef DEBUG_OUTPUT
    std::cerr << "rete.remove" << *wme << std::endl;
#endif

    {
      Locked_Node_Data locked_node_data(this);
      Locked_Network_Data locked_network_data(this, locked_node_data);

      const Outputs &outputs = locked_node_data.get_outputs();
      Unlocked_Network_Data::WMEs &wmes = locked_network_data.modify_wmes();

      auto found = wmes.positive.find(wme);
      if (found == wmes.positive.end()) {
        wmes.negative.emplace(wme);
        return;
      }

      auto found2 = locked_node_data.get_output_tokens().find(found->second);
      assert(found2 != locked_node_data.get_output_tokens().end());

      jobs.reserve(outputs.positive.size());
      for (auto &output : outputs.positive)
        jobs.emplace_back(std::make_shared<Message_Token_Remove>(output, sft, nullptr, *found2));

      locked_node_data.modify_output_tokens().erase(found2);
      wmes.positive.erase(found);
    }

    job_queue->give_many(std::move(jobs));
  }

  void Network::clear_wmes(const std::shared_ptr<Concurrency::Job_Queue> job_queue) {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    {
      Locked_Node_Data locked_node_data(this);
      Locked_Network_Data locked_network_data(this, locked_node_data);

      const Outputs &outputs = locked_node_data.get_outputs();
      Unlocked_Network_Data::WMEs &wmes = locked_network_data.modify_wmes();

      jobs.reserve(locked_node_data.get_output_tokens().size() * outputs.positive.size());
      for (auto &token : locked_node_data.get_output_tokens()) {
        for (auto &output : outputs.positive)
          jobs.emplace_back(std::make_shared<Message_Token_Remove>(output, sft, nullptr, token));
      }
      locked_node_data.modify_output_tokens().clear();
      wmes.positive.clear();
    }

    job_queue->give_many(std::move(jobs));
  }

  void Network::source_rule(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node_Action> action, const bool user_command) {
    const auto sft = shared_from_this();

    std::shared_ptr<Node_Action> excised;

    {
      Locked_Node_Data locked_node_data(this);
      Locked_Network_Data locked_network_data(this, locked_node_data);

      auto found = locked_network_data.modify_rules().find(action->get_name());
      if (found != locked_network_data.get_rules().end()) {
        //#ifndef NDEBUG
        //      std::cerr << "Rule '" << action->get_name() << "' replaced." << std::endl;
        //#endif
        assert(found->second != action);
        excised = found->second;
        if (user_command && m_printed_output != Printed_Output::None)
          std::cerr << '#';
        found->second = action;
      }
      else {
        //#ifndef NDEBUG
        //      std::cerr << "Rule '" << action->get_name() << "' sourced." << std::endl;
        //#endif
        locked_network_data.modify_rules()[action->get_name()] = action;
      }
      if (user_command && m_printed_output != Printed_Output::None)
        std::cerr << '*';
    }

    if(excised)
      job_queue->give_one(std::make_shared<Message_Decrement_Output_Count>(excised, sft, excised));
  }

  bool Network::operator==(const Node &rhs) const {
    return &rhs == this;
  }

}
