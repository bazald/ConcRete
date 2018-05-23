#include "Zeni/Rete/Network.hpp"

#include "Zeni/Concurrency/Thread_Pool.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Node_Filter.hpp"
#include "Zeni/Rete/Raven_Disconnect_Output.hpp"
#include "Zeni/Rete/Raven_Token_Insert.hpp"
#include "Zeni/Rete/Raven_Token_Remove.hpp"

#include <cassert>
#include <iostream>
#include <map>
#include <sstream>

namespace Zeni {

  namespace Rete {

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
      Tokens output_tokens;
    };

    class Network_Locked_Data_Const {
      Network_Locked_Data_Const(const Network_Locked_Data_Const &) = delete;
      Network_Locked_Data_Const & operator=(const Network_Locked_Data_Const &) = delete;

    public:
      Network_Locked_Data_Const(const Network * const &network)
        : m_lock(network->m_mutex),
        m_data(network->m_unlocked_data)
      {
      }

      const Network::Filters & get_filters() {
        return m_data->filters;
      }

      const std::unordered_map<std::string, std::shared_ptr<Node_Action>> & get_rules() {
        return m_data->rules;
      }

      int64_t get_rule_name_index() {
        return m_data->rule_name_index;
      }

      const Tokens & get_output_tokens() {
        return m_data->output_tokens;
      }

    private:
      Concurrency::Mutex::Lock m_lock;
      std::shared_ptr<Network_Unlocked_Data> m_data;
    };

    class Network_Locked_Data {
      Network_Locked_Data(const Network_Locked_Data &) = delete;
      Network_Locked_Data & operator=(const Network_Locked_Data &) = delete;

    public:
      Network_Locked_Data(Network * const &network)
        : m_lock(network->m_mutex),
        m_data(network->m_unlocked_data)
      {
      }

      Network::Filters & get_filters() {
        return m_data->filters;
      }

      std::unordered_map<std::string, std::shared_ptr<Node_Action>> & get_rules() {
        return m_data->rules;
      }

      int64_t & get_rule_name_index() {
        return m_data->rule_name_index;
      }

      Tokens & get_output_tokens() {
        return m_data->output_tokens;
      }

    private:
      Concurrency::Mutex::Lock m_lock;
      std::shared_ptr<Network_Unlocked_Data> m_data;
    };

    std::shared_ptr<const Network> Network::shared_from_this() const {
      return std::static_pointer_cast<const Network>(Concurrency::Maester::shared_from_this());
    }

    std::shared_ptr<Network> Network::shared_from_this() {
      return std::static_pointer_cast<Network>(Concurrency::Maester::shared_from_this());
    }

    Network::Instantiation::Instantiation(const std::shared_ptr<Network> &network)
      : m_network(network)
    {
    }

    std::shared_ptr<Network::Instantiation> Network::Instantiation::Create(const std::shared_ptr<Network> &network) {
      class Friendly_Network_Instantiation : public Instantiation {
      public:
        Friendly_Network_Instantiation(const std::shared_ptr<Network> &network)
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

    const std::shared_ptr<Network> & Network::Instantiation::get() {
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

    Network::Network(const Network::Printed_Output &printed_output)
      : m_thread_pool(std::make_shared<Concurrency::Thread_Pool>()), m_printed_output(printed_output)
    {
    }

    std::shared_ptr<Network::Instantiation> Network::Create(const Network::Printed_Output &printed_output) {
      class Friendly_Network : public Network {
      public:
        Friendly_Network(const Network::Printed_Output &printed_output)
          : Network(printed_output)
        {
        }
      };

      auto network_instantiation =  Instantiation::Create(std::make_shared<Friendly_Network>(printed_output));

      (*network_instantiation)->m_unlocked_data = std::make_shared<Network_Unlocked_Data>();

      return network_instantiation;
    }

    Network::Network(const std::shared_ptr<Concurrency::Thread_Pool> &thread_pool, const Printed_Output &printed_output)
      : m_thread_pool(thread_pool), m_printed_output(printed_output)
    {
    }

    std::shared_ptr<Network::Instantiation> Network::Create(const std::shared_ptr<Concurrency::Thread_Pool> &thread_pool, const Network::Printed_Output &printed_output) {
      class Friendly_Network : public Network {
      public:
        Friendly_Network(const std::shared_ptr<Concurrency::Thread_Pool> &thread_pool, const Network::Printed_Output &printed_output)
          : Network(thread_pool, printed_output)
        {
        }
      };

      return Instantiation::Create(std::make_shared<Friendly_Network>(thread_pool, printed_output));
    }

    void Network::Destroy() {
      m_thread_pool->get_Job_Queue()->wait_for_completion();
      m_thread_pool.reset();
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

    void Network::set_rule_name_index(const int64_t &rule_name_index_) {
      Network_Locked_Data locked_data(this);

      locked_data.get_rule_name_index() = rule_name_index_;
    }

    Network::Node_Sharing Network::get_Node_Sharing() const {
      return m_node_sharing;
    }

    Network::Printed_Output Network::get_Printed_Output() const {
      return m_printed_output;
    }

    void Network::excise_all() {
      Network_Locked_Data locked_data(this);

      for (auto &rule : locked_data.get_rules())
        rule.second->get_parent()->disconnect_output(shared_from_this(), rule.second);

      locked_data.get_rules().clear();
    }

    void Network::receive(Concurrency::Job_Queue &job_queue, const Concurrency::Raven &raven) {
      const auto &disconnect_output = dynamic_cast<const Raven_Disconnect_Output &>(raven);

      Network_Locked_Data locked_data(this);

      if (disconnect_output.get_output()->get_output_count() == 0) {
        const auto found = locked_data.get_filters().find(std::dynamic_pointer_cast<Node_Filter>(disconnect_output.get_output()));
        assert(found != locked_data.get_filters().end());
        locked_data.get_filters().erase(found);
      }
    }

    std::shared_ptr<Node_Filter> Network::find_filter_and_increment_output_count(const std::shared_ptr<Node_Filter> &filter) {
      Network_Locked_Data locked_data(this);

      for (auto &existing_filter : locked_data.get_filters()) {
        if (*existing_filter == *filter) {
          existing_filter->increment_output_count();
          return existing_filter;
        }
      }
      
      return nullptr;
    }

    void Network::source_filter(const std::shared_ptr<Node_Filter> &filter) {
      const auto sft = shared_from_this();

      Network_Locked_Data locked_data(this);

      locked_data.get_filters().insert(filter);
      for (auto &output_token : locked_data.get_output_tokens())
        m_thread_pool->get_Job_Queue()->give(std::make_shared<Raven_Token_Insert>(filter, sft, nullptr, output_token));
    }

    void Network::excise_rule(const std::string &name, const bool &user_command) {
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
        locked_data.get_rules().erase(found);
        
        action->get_parent()->disconnect_output(shared_from_this(), action);
        if (user_command)
          std::cerr << '#';
      }
    }

    std::string Network::next_rule_name(const std::string &prefix) {
      Network_Locked_Data locked_data(this);

      std::ostringstream oss;
      do {
        oss.str("");
        oss << prefix << ++locked_data.get_rule_name_index();
      } while (locked_data.get_rules().find(oss.str()) != locked_data.get_rules().end());
      return oss.str();
    }

    std::shared_ptr<Node_Action> Network::unname_rule(const std::string &name, const bool &user_command) {
      Network_Locked_Data locked_data(this);

      std::shared_ptr<Node_Action> ptr;
      auto found = locked_data.get_rules().find(name);
      if (found != locked_data.get_rules().end()) {
        ptr = found->second;
        locked_data.get_rules().erase(found);
        if (user_command)
          std::cerr << '#';
      }
      return ptr;
    }

    void Network::insert_wme(const std::shared_ptr<const WME> &wme) {
      const auto sft = shared_from_this();
      const auto output_token = std::make_shared<Token>(wme);

      Network_Locked_Data locked_data(this);

      locked_data.get_output_tokens().insert(output_token);
#ifdef DEBUG_OUTPUT
      std::cerr << "rete.insert" << *wme << std::endl;
#endif
      for (auto &filter : locked_data.get_filters())
        m_thread_pool->get_Job_Queue()->give(std::make_shared<Raven_Token_Insert>(filter, sft, nullptr, output_token));
    }

    void Network::remove_wme(const std::shared_ptr<const WME> &wme) {
      const auto sft = shared_from_this();
      const auto output_token = std::make_shared<Token>(wme);

      Network_Locked_Data locked_data(this);

      auto found = locked_data.get_output_tokens().find(output_token);
      assert(found != locked_data.get_output_tokens().end());
      locked_data.get_output_tokens().erase(found);

#ifdef DEBUG_OUTPUT
      std::cerr << "rete.remove" << *wme << std::endl;
#endif
      for (auto &filter : locked_data.get_filters())
        m_thread_pool->get_Job_Queue()->give(std::static_pointer_cast<Concurrency::Job>(std::make_shared<Raven_Token_Remove>(filter, sft, nullptr, output_token)));
    }

    void Network::clear_wmes() {
      const auto sft = shared_from_this();

      Network_Locked_Data locked_data(this);

      for (auto &output_token : locked_data.get_output_tokens()) {
        for (auto &filter : locked_data.get_filters())
          m_thread_pool->get_Job_Queue()->give(std::static_pointer_cast<Concurrency::Job>(std::make_shared<Raven_Token_Remove>(filter, sft, nullptr, output_token)));
      }
      locked_data.get_output_tokens().clear();
    }

    void Network::source_rule(const std::shared_ptr<Node_Action> &action, const bool &user_command) {
      const auto sft = shared_from_this();

      Network_Locked_Data locked_data(this);

      auto found = locked_data.get_rules().find(action->get_name());
      if (found == locked_data.get_rules().end()) {
        //#ifndef NDEBUG
        //      std::cerr << "Rule '" << action->get_name() << "' sourced." << std::endl;
        //#endif
        locked_data.get_rules()[action->get_name()] = action;
      }
      else {
        //#ifndef NDEBUG
        //      std::cerr << "Rule '" << action->get_name() << "' replaced." << std::endl;
        //#endif
        assert(found->second != action);
        found->second->get_parent()->disconnect_output(sft, found->second);
        if (user_command && m_printed_output != Printed_Output::None)
          std::cerr << '#';
        found->second = action;
      }
      if (user_command && m_printed_output != Printed_Output::None)
        std::cerr << '*';
    }

  }

}
