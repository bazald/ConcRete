#include "Zeni/Rete/Network.hpp"

#include "Zeni/Concurrency/Thread_Pool.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Node_Filter.hpp"
#include "Zeni/Rete/Raven_Disconnect_Output.hpp"
#include "Zeni/Rete/Raven_Token_Insert.hpp"
#include "Zeni/Rete/Raven_Token_Remove.hpp"
#include "Zeni/Rete/Token_Alpha.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <string_view>

namespace Zeni::Rete {

  class Network_Unlocked_Data {
    Network_Unlocked_Data(const Network_Unlocked_Data &) = delete;
    Network_Unlocked_Data & operator=(const Network_Unlocked_Data &) = delete;

    friend Network_Locked_Data_Const;
    friend Network_Locked_Data;

  public:
    Network_Unlocked_Data() {}

  private:
    Network::Filters filters;
    std::unordered_map<std::string, std::shared_ptr<Node_Action>> rules;
    int64_t rule_name_index = 0;
    std::unordered_multimap<std::shared_ptr<const WME>, std::shared_ptr<const Token>, hash_deref<WME>, compare_deref_eq> working_memory;
  };

  class Network_Locked_Data;

  class Network_Locked_Data_Const {
    Network_Locked_Data_Const(const Network_Locked_Data_Const &) = delete;
    Network_Locked_Data_Const & operator=(const Network_Locked_Data_Const &) = delete;

    friend Network_Locked_Data;

  public:
    Network_Locked_Data_Const(const Network *network)
      : m_lock(network->m_mutex),
      m_data(network->m_unlocked_data)
    {
    }

    const Network::Filters & get_filters() const {
      return m_data->filters;
    }

    const std::unordered_map<std::string, std::shared_ptr<Node_Action>> & get_rules() const {
      return m_data->rules;
    }

    int64_t get_rule_name_index() const {
      return m_data->rule_name_index;
    }

    const std::unordered_multimap<std::shared_ptr<const WME>, std::shared_ptr<const Token>, hash_deref<WME>, compare_deref_eq> & get_working_memory() const {
      return m_data->working_memory;
    }

  private:
    Concurrency::Mutex::Lock m_lock;
    std::shared_ptr<Network_Unlocked_Data> m_data;
  };

  class Network_Locked_Data : public Network_Locked_Data_Const {
    Network_Locked_Data(const Network_Locked_Data &) = delete;
    Network_Locked_Data & operator=(const Network_Locked_Data &) = delete;

  public:
    Network_Locked_Data(Network *network)
      : Network_Locked_Data_Const(network)
    {
    }

    Network::Filters & modify_filters() {
      return m_data->filters;
    }

    std::unordered_map<std::string, std::shared_ptr<Node_Action>> & modify_rules() {
      return m_data->rules;
    }

    int64_t & modify_rule_name_index() {
      return m_data->rule_name_index;
    }

    std::unordered_multimap<std::shared_ptr<const WME>, std::shared_ptr<const Token>, hash_deref<WME>, compare_deref_eq> & modify_working_memory() {
      return m_data->working_memory;
    }
  };

  std::shared_ptr<const Network> Network::shared_from_this() const {
    return std::static_pointer_cast<const Network>(Concurrency::Maester::shared_from_this());
  }

  std::shared_ptr<Network> Network::shared_from_this() {
    return std::static_pointer_cast<Network>(Concurrency::Maester::shared_from_this());
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
    : m_thread_pool(std::make_shared<Concurrency::Thread_Pool>()),
    m_unlocked_data(std::make_shared<Network_Unlocked_Data>()),
    m_printed_output(printed_output)
  {
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

  Network::Network(const Printed_Output printed_output, const std::shared_ptr<Concurrency::Thread_Pool> &thread_pool)
    : m_thread_pool(thread_pool),
    m_unlocked_data(std::make_shared<Network_Unlocked_Data>()),
    m_printed_output(printed_output)
  {
  }

  std::shared_ptr<Network::Instantiation> Network::Create(const Network::Printed_Output printed_output, const std::shared_ptr<Concurrency::Thread_Pool> thread_pool) {
    class Friendly_Network : public Network {
    public:
      Friendly_Network(const Network::Printed_Output &printed_output, const std::shared_ptr<Concurrency::Thread_Pool> &thread_pool)
        : Network(printed_output, thread_pool)
      {
      }
    };

    return Instantiation::Create(std::make_shared<Friendly_Network>(printed_output, thread_pool));
  }

  void Network::Destroy() {
    excise_all();
    m_thread_pool->get_Job_Queue()->wait_for_completion();
  }

  Network::~Network()
  {
  }

  std::shared_ptr<Concurrency::Job_Queue> Network::get_Job_Queue() const {
    return m_thread_pool->get_Job_Queue();
  }

  std::shared_ptr<Concurrency::Thread_Pool> Network::get_Thread_Pool() const {
    return m_thread_pool;
  }

  std::shared_ptr<Node_Action> Network::get_rule(const std::string &name) const {
    Network_Locked_Data_Const locked_data(this);

    const auto found = locked_data.get_rules().find(name);
    if (found != locked_data.get_rules().end())
      return found->second;

    return nullptr;
  }

  std::set<std::string> Network::get_rule_names() const {
    Network_Locked_Data_Const locked_data(this);

    std::set<std::string> rv;
    for (auto rule : locked_data.get_rules())
      rv.insert(rule.first);
    return rv;
  }

  int64_t Network::get_rule_name_index() const {
    Network_Locked_Data_Const locked_data(this);

    return locked_data.get_rule_name_index();
  }

  void Network::set_rule_name_index(const int64_t rule_name_index_) {
    Network_Locked_Data locked_data(this);

    locked_data.modify_rule_name_index() = rule_name_index_;
  }

  Network::Node_Sharing Network::get_Node_Sharing() const {
    return m_node_sharing;
  }

  Network::Printed_Output Network::get_Printed_Output() const {
    return m_printed_output;
  }

  void Network::excise_all() {
    const auto sft = shared_from_this();
    std::unordered_map<std::string, std::shared_ptr<Node_Action>> rules;

    {
      Network_Locked_Data locked_data(this);
      rules.swap(locked_data.modify_rules());
    }

    std::vector<std::shared_ptr<Concurrency::Job>> jobs;
    jobs.reserve(rules.size());
    for (auto &rule : rules)
      jobs.emplace_back(std::make_shared<Raven_Disconnect_Output>(rule.second->get_input(), sft, rule.second));

    get_Job_Queue()->give_many(std::move(jobs));
  }

  void Network::disconnect_output(const std::shared_ptr<Network> network, const std::shared_ptr<const Node> output) {
    std::vector<std::shared_ptr<Concurrency::Job>> jobs;

    {
      Network_Locked_Data locked_data(this);

      const auto found = locked_data.get_filters().find(
        std::const_pointer_cast<Node_Filter>(std::dynamic_pointer_cast<const Node_Filter>(output)));
      assert(found != locked_data.get_filters().end());
      locked_data.modify_filters().erase(found);

      jobs.reserve(locked_data.get_working_memory().size());
      for (auto &wme_token : locked_data.get_working_memory())
        jobs.emplace_back(std::make_shared<Raven_Token_Remove>(std::const_pointer_cast<Node>(output), network, nullptr, wme_token.second));
    }

    network->get_Job_Queue()->give_many(std::move(jobs));
  }

  bool Network::receive(const Raven_Token_Insert &) {
    return false;
  }

  bool Network::receive(const Raven_Token_Remove &) {
    return false;
  }

  std::shared_ptr<Node_Filter> Network::find_filter_and_increment_output_count(const WME &wme) {
    Network_Locked_Data locked_data(this);

    for (auto &existing_filter : locked_data.get_filters()) {
      if (existing_filter->get_wme() == wme) {
        existing_filter->increment_output_count();
        return existing_filter;
      }
    }

    return nullptr;
  }

  void Network::source_filter(const std::shared_ptr<Node_Filter> filter) {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::Job>> jobs;

    {
      Network_Locked_Data locked_data(this);
      locked_data.modify_filters().insert(filter);
      jobs.reserve(locked_data.get_working_memory().size());
      for (auto &wme_token : locked_data.get_working_memory())
        jobs.emplace_back(std::make_shared<Raven_Token_Insert>(filter, sft, nullptr, wme_token.second));
    }

    m_thread_pool->get_Job_Queue()->give_many(std::move(jobs));
  }

  void Network::excise_rule(const std::string &name, const bool user_command) {
    Network_Locked_Data locked_data(this);

    auto found = locked_data.get_rules().find(name);
    if (found == locked_data.get_rules().end()) {
      //#ifndef NDEBUG
      //      std::cerr << "Rule '" << name << "' not found." << std::endl;
      //#endif
    }
    else {
      //#ifndef NDEBUG
      //      std::cerr << "Rule '" << name << "' excised." << std::endl;
      //#endif
      auto action = found->second;
      locked_data.modify_rules().erase(found);

      action->get_input()->disconnect_output(shared_from_this(), action);
      if (user_command)
        std::cerr << '#';
    }
  }

  std::string Network::next_rule_name(const std::string_view prefix) {
    Network_Locked_Data locked_data(this);

    std::ostringstream oss;
    do {
      oss.str("");
      oss << prefix << ++locked_data.modify_rule_name_index();
    } while (locked_data.get_rules().find(oss.str()) != locked_data.get_rules().end());
    return oss.str();
  }

  std::shared_ptr<Node_Action> Network::unname_rule(const std::string &name, const bool user_command) {
    Network_Locked_Data locked_data(this);

    std::shared_ptr<Node_Action> ptr;
    auto found = locked_data.get_rules().find(name);
    if (found != locked_data.get_rules().end()) {
      ptr = found->second;
      locked_data.modify_rules().erase(found);
      if (user_command)
        std::cerr << '#';
    }
    return ptr;
  }

  void Network::insert_wme(const std::shared_ptr<const WME> wme) {
    const auto sft = shared_from_this();
    const auto output_token = std::make_shared<Token_Alpha>(wme);
    std::vector<std::shared_ptr<Concurrency::Job>> jobs;

#ifdef DEBUG_OUTPUT
    std::cerr << "rete.insert" << *wme << std::endl;
#endif

    {
      Network_Locked_Data locked_data(this);

      locked_data.modify_working_memory().insert(std::make_pair(wme, output_token));
      jobs.reserve(locked_data.get_filters().size());
      for (auto &filter : locked_data.get_filters())
        jobs.emplace_back(std::make_shared<Raven_Token_Insert>(filter, sft, nullptr, output_token));
    }

    m_thread_pool->get_Job_Queue()->give_many(std::move(jobs));
  }

  void Network::remove_wme(const std::shared_ptr<const WME> wme) {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::Job>> jobs;

#ifdef DEBUG_OUTPUT
    std::cerr << "rete.remove" << *wme << std::endl;
#endif

    {
      Network_Locked_Data locked_data(this);

      auto found = locked_data.get_working_memory().find(wme);
      assert(found != locked_data.get_working_memory().end());

      jobs.reserve(locked_data.get_filters().size());
      for (auto &filter : locked_data.get_filters())
        jobs.emplace_back(std::make_shared<Raven_Token_Remove>(filter, sft, nullptr, found->second));
      locked_data.modify_working_memory().erase(found);
    }

    m_thread_pool->get_Job_Queue()->give_many(std::move(jobs));
  }

  void Network::clear_wmes() {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::Job>> jobs;

    {
      Network_Locked_Data locked_data(this);

      jobs.reserve(locked_data.get_working_memory().size() * locked_data.get_filters().size());
      for (auto &wme_token : locked_data.get_working_memory()) {
        for (auto &filter : locked_data.get_filters())
          jobs.emplace_back(std::make_shared<Raven_Token_Remove>(filter, sft, nullptr, wme_token.second));
      }
      locked_data.modify_working_memory().clear();
    }

    m_thread_pool->get_Job_Queue()->give_many(std::move(jobs));
  }

  void Network::source_rule(const std::shared_ptr<Node_Action> action, const bool user_command) {
    const auto sft = shared_from_this();

    Network_Locked_Data locked_data(this);

    auto found = locked_data.modify_rules().find(action->get_name());
    if (found == locked_data.get_rules().end()) {
      //#ifndef NDEBUG
      //      std::cerr << "Rule '" << action->get_name() << "' sourced." << std::endl;
      //#endif
      locked_data.modify_rules()[action->get_name()] = action;
    }
    else {
      //#ifndef NDEBUG
      //      std::cerr << "Rule '" << action->get_name() << "' replaced." << std::endl;
      //#endif
      assert(found->second != action);
      found->second->get_input()->disconnect_output(sft, found->second);
      if (user_command && m_printed_output != Printed_Output::None)
        std::cerr << '#';
      found->second = action;
    }
    if (user_command && m_printed_output != Printed_Output::None)
      std::cerr << '*';
  }

}
