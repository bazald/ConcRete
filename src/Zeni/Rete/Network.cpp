#include "Zeni/Rete/Network.hpp"

#include "Zeni/Concurrency/Thread_Pool.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Node_Filter.hpp"
#include "Zeni/Rete/Raven_Token_Insert.hpp"
#include "Zeni/Rete/Raven_Token_Remove.hpp"

#include <cassert>
#include <iostream>
#include <map>
#include <sstream>

namespace Zeni {

  namespace Rete {

    Network::Network(const Network::Printed_Output &printed_output)
      : m_thread_pool(std::make_shared<Concurrency::Thread_Pool>()), m_printed_output(printed_output)
    {
    }

    std::shared_ptr<Network> Network::Create(const Network::Printed_Output &printed_output) {
      class Friendly_Network : public Network {
      public:
        Friendly_Network(const Network::Printed_Output &printed_output)
          : Network(printed_output)
        {
        }
      };

      return std::make_shared<Friendly_Network>(printed_output);
    }

    Network::Network(const std::shared_ptr<Concurrency::Thread_Pool> &thread_pool, const Printed_Output &printed_output)
      : m_thread_pool(thread_pool), m_printed_output(printed_output)
    {
    }

    std::shared_ptr<Network> Network::Create(const std::shared_ptr<Concurrency::Thread_Pool> &thread_pool, const Network::Printed_Output &printed_output) {
      class Friendly_Network : public Network {
      public:
        Friendly_Network(const std::shared_ptr<Concurrency::Thread_Pool> &thread_pool, const Network::Printed_Output &printed_output)
          : Network(thread_pool, printed_output)
        {
        }
      };

      return std::make_shared<Friendly_Network>(thread_pool, printed_output);
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
      Concurrency::Mutex::Lock lock(m_mutex);

      const auto found = m_rules.find(name);
      if (found != m_rules.end())
        return found->second;
      return nullptr;
    }

    std::set<std::string> Network::get_rule_names() const {
      Concurrency::Mutex::Lock lock(m_mutex);

      std::set<std::string> rv;
      for (auto rule : m_rules)
        rv.insert(rule.first);
      return rv;
    }

    int64_t Network::get_rule_name_index() const {
      Concurrency::Mutex::Lock lock(m_mutex);

      return m_rule_name_index;
    }

    void Network::set_rule_name_index(const int64_t &rule_name_index_) {
      Concurrency::Mutex::Lock lock(m_mutex);

      m_rule_name_index = rule_name_index_;
    }

    Network::Node_Sharing Network::get_Node_Sharing() const {
      return m_node_sharing;
    }

    Network::Printed_Output Network::get_Printed_Output() const {
      return m_printed_output;
    }

    void Network::excise_all() {
      Concurrency::Mutex::Lock lock(m_mutex);

      m_filters.clear();
      m_rules.clear();
    }

    std::shared_ptr<Node_Filter> Network::find_filter(const std::shared_ptr<Node_Filter> &filter) const {
      Concurrency::Mutex::Lock lock(m_mutex);

      for (auto &existing_filter : m_filters) {
        if (*existing_filter == *filter)
          return existing_filter;
      }
      
      return nullptr;
    }

    void Network::source_filter(const std::shared_ptr<Node_Filter> &filter) {
      Concurrency::Mutex::Lock lock(m_mutex);

      m_filters.insert(filter);
      const auto sft = shared_from_this();
      for (auto &wme : m_working_memory.get_wmes())
        m_thread_pool->get_Job_Queue()->give(std::make_shared<Raven_Token_Insert>(filter, sft, nullptr, std::make_shared<Token>(wme)));
    }

    void Network::excise_filter(const std::shared_ptr<Node_Filter> &filter) {
      Concurrency::Mutex::Lock lock(m_mutex);

      m_filters.erase(m_filters.find(filter));
    }

    void Network::excise_rule(const std::string &name, const bool &user_command) {
      Concurrency::Mutex::Lock lock(m_mutex);

      auto found = m_rules.find(name);
      if (found == m_rules.end()) {
        //#ifndef NDEBUG
        //      std::cerr << "Rule '" << name << "' not found." << std::endl;
        //#endif
      }
      else {
        //#ifndef NDEBUG
        //      std::cerr << "Rule '" << name << "' excised." << std::endl;
        //#endif
        auto action = found->second;
        m_rules.erase(found);
        
        action->get_parent()->disconnect_output(shared_from_this(), action);
        if (user_command)
          std::cerr << '#';
      }
    }

    std::string Network::next_rule_name(const std::string &prefix) {
      Concurrency::Mutex::Lock lock(m_mutex);

      std::ostringstream oss;
      do {
        oss.str("");
        oss << prefix << ++m_rule_name_index;
      } while (m_rules.find(oss.str()) != m_rules.end());
      return oss.str();
    }

    std::shared_ptr<Node_Action> Network::unname_rule(const std::string &name, const bool &user_command) {
      Concurrency::Mutex::Lock lock(m_mutex);

      std::shared_ptr<Node_Action> ptr;
      auto found = m_rules.find(name);
      if (found != m_rules.end()) {
        ptr = found->second;
        m_rules.erase(found);
        if (user_command)
          std::cerr << '#';
      }
      return ptr;
    }

    void Network::insert_wme(const std::shared_ptr<const WME> &wme) {
      Concurrency::Mutex::Lock lock(m_mutex);

      if (m_working_memory.get_wmes().find(wme) != m_working_memory.get_wmes().end()) {
#ifdef DEBUG_OUTPUT
        std::cerr << "rete.already_inserted" << *wme << std::endl;
#endif
        return;
      }

      //const auto wme_clone = std::make_shared<WME>(std::shared_ptr<const Symbol>(wme->symbols[0]->clone()), std::shared_ptr<const Symbol>(wme->symbols[1]->clone()), std::shared_ptr<const Symbol>(wme->symbols[2]->clone()));

      m_working_memory.get_wmes().insert(wme);
#ifdef DEBUG_OUTPUT
      std::cerr << "rete.insert" << *wme << std::endl;
#endif
      const auto sft = shared_from_this();
      for (auto &filter : m_filters)
        m_thread_pool->get_Job_Queue()->give(std::make_shared<Raven_Token_Insert>(filter, sft, nullptr, std::make_shared<Token>(wme)));
    }

    void Network::remove_wme(const std::shared_ptr<const WME> &wme) {
      Concurrency::Mutex::Lock lock(m_mutex);

      auto found = m_working_memory.get_wmes().find(wme);

      if (found == m_working_memory.get_wmes().end()) {
#ifdef DEBUG_OUTPUT
        std::cerr << "rete.already_removed" << *wme << std::endl;
#endif
        return;
      }

      m_working_memory.get_wmes().erase(found);
#ifdef DEBUG_OUTPUT
      std::cerr << "rete.remove" << *wme << std::endl;
#endif
      const auto sft = shared_from_this();
      for (auto &filter : m_filters)
        m_thread_pool->get_Job_Queue()->give(std::static_pointer_cast<Concurrency::Job>(std::make_shared<Raven_Token_Remove>(filter, sft, nullptr, std::make_shared<Token>(wme))));
    }

    void Network::clear_wmes() {
      Concurrency::Mutex::Lock lock(m_mutex);

      const auto sft = shared_from_this();
      for (auto &wme : m_working_memory.get_wmes()) {
        for (auto &filter : m_filters)
          m_thread_pool->get_Job_Queue()->give(std::static_pointer_cast<Concurrency::Job>(std::make_shared<Raven_Token_Remove>(filter, sft, nullptr, std::make_shared<Token>(wme))));
      }
      m_working_memory.get_wmes().clear();
    }

    void Network::source_rule(const std::shared_ptr<Node_Action> &action, const bool &user_command) {
      Concurrency::Mutex::Lock lock(m_mutex);

      const auto sft = shared_from_this();

      auto found = m_rules.find(action->get_name());
      if (found == m_rules.end()) {
        //#ifndef NDEBUG
        //      std::cerr << "Rule '" << action->get_name() << "' sourced." << std::endl;
        //#endif
        m_rules[action->get_name()] = action;
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
