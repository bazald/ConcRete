#ifndef ZENI_RETE_NETWORK_H
#define ZENI_RETE_NETWORK_H

#include "Zeni/Concurrency/Maester.hpp"
#include "Zeni/Rete/Linkage.hpp"

#include <set>
#include <unordered_set>

namespace Zeni {

  namespace Concurrency {

    class Job_Queue;
    class Thread_Pool;

  }

  namespace Rete {

    class Network_Locked_Data;
    class Network_Locked_Data_Const;
    class Network_Unlocked_Data;
    class Node_Action;
    class Node_Filter;
    class WME;

    class Network : public Concurrency::Maester {
      Network(const Network &) = delete;
      Network & operator=(const Network &) = delete;

      friend class Network_Locked_Data;
      friend class Network_Locked_Data_Const;

    protected:
      ZENI_RETE_LINKAGE std::shared_ptr<const Network> shared_from_this() const;
      ZENI_RETE_LINKAGE std::shared_ptr<Network> shared_from_this();

    public:
      class Instantiation : public std::enable_shared_from_this<Instantiation> {
        Instantiation(const Instantiation &) = delete;
        Instantiation & operator=(const Instantiation &) = delete;

        ZENI_RETE_LINKAGE Instantiation(const std::shared_ptr<Network> &network);

      public:
        ZENI_RETE_LINKAGE static std::shared_ptr<Instantiation> Create(const std::shared_ptr<Network> &network);

        ZENI_RETE_LINKAGE ~Instantiation();

        ZENI_RETE_LINKAGE std::shared_ptr<const Network> get() const;
        ZENI_RETE_LINKAGE const std::shared_ptr<Network> & get();

        ZENI_RETE_LINKAGE const Network * operator*() const;
        ZENI_RETE_LINKAGE Network * operator*();
        ZENI_RETE_LINKAGE const Network * operator->() const;
        ZENI_RETE_LINKAGE Network * operator->();

      private:
        const std::shared_ptr<Network> m_network;
      };

      enum class ZENI_RETE_LINKAGE Node_Sharing { Enabled, Disabled };
      enum class ZENI_RETE_LINKAGE Printed_Output { Normal, None };

      typedef std::unordered_set<std::shared_ptr<Node_Filter>> Filters;

    private:
      Network(const Printed_Output &printed_output);
      Network(const Printed_Output &printed_output, const std::shared_ptr<Concurrency::Thread_Pool> &thread_pool);

      ZENI_RETE_LINKAGE void Destroy();

    public:
      ZENI_RETE_LINKAGE static std::shared_ptr<Instantiation> Create(const Printed_Output &printed_output = Printed_Output::Normal);
      ZENI_RETE_LINKAGE static std::shared_ptr<Instantiation> Create(const Printed_Output &printed_output, const std::shared_ptr<Concurrency::Thread_Pool> &thread_pool);

      ZENI_RETE_LINKAGE ~Network();

      ZENI_RETE_LINKAGE std::shared_ptr<Concurrency::Job_Queue> get_Job_Queue() const;
      ZENI_RETE_LINKAGE std::shared_ptr<Concurrency::Thread_Pool> get_Thread_Pool() const;
      ZENI_RETE_LINKAGE std::shared_ptr<Node_Action> get_rule(const std::string &name) const;
      ZENI_RETE_LINKAGE std::set<std::string> get_rule_names() const;
      ZENI_RETE_LINKAGE int64_t get_rule_name_index() const;
      ZENI_RETE_LINKAGE void set_rule_name_index(const int64_t &rule_name_index_);
      ZENI_RETE_LINKAGE Node_Sharing get_Node_Sharing() const;
      ZENI_RETE_LINKAGE Printed_Output get_Printed_Output() const;

      ZENI_RETE_LINKAGE void receive(Concurrency::Job_Queue &job_queue, const Concurrency::Raven &raven) override;

      ZENI_RETE_LINKAGE std::shared_ptr<Node_Filter> find_filter_and_increment_output_count(const std::shared_ptr<Node_Filter> &filter);

      ZENI_RETE_LINKAGE void source_rule(const std::shared_ptr<Node_Action> &action, const bool &user_command);
      ZENI_RETE_LINKAGE void excise_all();
      ZENI_RETE_LINKAGE void source_filter(const std::shared_ptr<Node_Filter> &filter);
      ZENI_RETE_LINKAGE void excise_rule(const std::string &name, const bool &user_command);
      ZENI_RETE_LINKAGE std::string next_rule_name(const std::string &prefix);
      ZENI_RETE_LINKAGE std::shared_ptr<Node_Action> unname_rule(const std::string &name, const bool &user_command);

      ZENI_RETE_LINKAGE void insert_wme(const std::shared_ptr<const WME> &wme);
      ZENI_RETE_LINKAGE void remove_wme(const std::shared_ptr<const WME> &wme);
      ZENI_RETE_LINKAGE void clear_wmes();

    private:
      const std::shared_ptr<Concurrency::Thread_Pool> m_thread_pool;
      const std::shared_ptr<Network_Unlocked_Data> m_unlocked_data;

      // Options
      const Node_Sharing m_node_sharing = Node_Sharing::Enabled;
      const Printed_Output m_printed_output;
    };

  }

}

#endif
