#ifndef ZENI_RETE_NETWORK_H
#define ZENI_RETE_NETWORK_H

#include "Zeni/Concurrency/Mutex.hpp"
#include "Working_Memory.hpp"

#include <chrono>
#include <set>
#include <unordered_map>

namespace Zeni {

  namespace Concurrency {

    class Job_Queue;
    class Thread_Pool;

  }

  namespace Rete {

    class Node_Action;
    class Node_Filter;

    class Network : public std::enable_shared_from_this<Network> {
      Network(const std::shared_ptr<Network> &);
      const std::shared_ptr<Network> & operator=(const std::shared_ptr<Network> &);

    public:
      enum class ZENI_RETE_LINKAGE Node_Sharing { Enabled, Disabled };
      enum class ZENI_RETE_LINKAGE Printed_Output { Normal, None };

      typedef std::unordered_set<std::shared_ptr<Node_Filter>> Filters;

    private:
      Network(const Printed_Output &printed_output);
      Network(const std::shared_ptr<Concurrency::Thread_Pool> &thread_pool, const Printed_Output &printed_output);

    public:
      ZENI_RETE_LINKAGE static std::shared_ptr<Network> Create(const Printed_Output &printed_output = Printed_Output::Normal);
      ZENI_RETE_LINKAGE static std::shared_ptr<Network> Create(const std::shared_ptr<Concurrency::Thread_Pool> &thread_pool, const Printed_Output &printed_output = Printed_Output::Normal);

      ZENI_RETE_LINKAGE void Destroy();
      ZENI_RETE_LINKAGE ~Network();

      ZENI_RETE_LINKAGE std::shared_ptr<Concurrency::Job_Queue> get_Job_Queue() const;
      ZENI_RETE_LINKAGE std::shared_ptr<Concurrency::Thread_Pool> get_Thread_Pool() const;
      ZENI_RETE_LINKAGE std::shared_ptr<Node_Action> get_rule(const std::string &name) const;
      ZENI_RETE_LINKAGE std::set<std::string> get_rule_names() const;
      ZENI_RETE_LINKAGE int64_t get_rule_name_index() const;
      ZENI_RETE_LINKAGE void set_rule_name_index(const int64_t &rule_name_index_);
      ZENI_RETE_LINKAGE Node_Sharing get_Node_Sharing() const;
      ZENI_RETE_LINKAGE Printed_Output get_Printed_Output() const;

      ZENI_RETE_LINKAGE std::shared_ptr<Node_Filter> find_filter(const std::shared_ptr<Node_Filter> &filter) const;

      ZENI_RETE_LINKAGE void source_rule(const std::shared_ptr<Node_Action> &action, const bool &user_command);
      ZENI_RETE_LINKAGE void excise_all();
      ZENI_RETE_LINKAGE void source_filter(const std::shared_ptr<Node_Filter> &filter);
      ZENI_RETE_LINKAGE void excise_filter(const std::shared_ptr<Node_Filter> &filter);
      ZENI_RETE_LINKAGE void excise_rule(const std::string &name, const bool &user_command);
      ZENI_RETE_LINKAGE std::string next_rule_name(const std::string &prefix);
      ZENI_RETE_LINKAGE std::shared_ptr<Node_Action> unname_rule(const std::string &name, const bool &user_command);

      ZENI_RETE_LINKAGE void insert_wme(const std::shared_ptr<const WME> &wme);
      ZENI_RETE_LINKAGE void remove_wme(const std::shared_ptr<const WME> &wme);
      ZENI_RETE_LINKAGE void clear_wmes();

      ZENI_RETE_LINKAGE void rete_print_rules(std::ostream &os) const;
      ZENI_RETE_LINKAGE void rete_print_firing_counts(std::ostream &os) const;
      ZENI_RETE_LINKAGE void rete_print_matches(std::ostream &os) const;

    private:
      std::shared_ptr<Concurrency::Thread_Pool> m_thread_pool;
      mutable Concurrency::Mutex m_mutex;

      Filters m_filters;
      std::unordered_map<std::string, std::shared_ptr<Node_Action>> m_rules;
      int64_t m_rule_name_index = 0;
      Working_Memory m_working_memory;

      // Options

      Node_Sharing m_node_sharing = Node_Sharing::Enabled;
      Printed_Output m_printed_output;
    };

  }

}

#endif
