#ifndef ZENI_RETE_NETWORK_H
#define ZENI_RETE_NETWORK_H

#include "Agenda.hpp"
#include "Node_Action.hpp"
#include "Node_Predicate.hpp"
#include "Working_Memory.hpp"

#include <chrono>
#include <unordered_map>

namespace Zeni {

  namespace Concurrency {

    class Job_Queue;
    class Thread_Pool;

  }

  namespace Rete {

    class Network : public std::enable_shared_from_this<Network> {
      Network(const std::shared_ptr<Network> &);
      const std::shared_ptr<Network> & operator=(const std::shared_ptr<Network> &);

    public:
      class ZENI_RETE_LINKAGE CPU_Accumulator {
        CPU_Accumulator(const CPU_Accumulator &);
        CPU_Accumulator operator=(const CPU_Accumulator &);

      public:
        CPU_Accumulator(const std::shared_ptr<Network> &network_);
        ~CPU_Accumulator();

      private:
        const std::shared_ptr<Network> &network;
      };

      enum class ZENI_RETE_LINKAGE Node_Sharing { Enabled, Disabled };
      enum class ZENI_RETE_LINKAGE Printed_Output { Normal, None };

    private:
      Network(const Printed_Output &printed_output);
      Network(const std::shared_ptr<Concurrency::Thread_Pool> &thread_pool, const Printed_Output &printed_output);

    public:
      ZENI_RETE_LINKAGE static std::shared_ptr<Network> Create(const Printed_Output &printed_output = Printed_Output::Normal);
      ZENI_RETE_LINKAGE static std::shared_ptr<Network> Create(const std::shared_ptr<Concurrency::Thread_Pool> &thread_pool, const Printed_Output &printed_output = Printed_Output::Normal);
      ZENI_RETE_LINKAGE void Destroy();
      ZENI_RETE_LINKAGE ~Network();

      ZENI_RETE_LINKAGE std::shared_ptr<Node_Action> make_action(const std::string &name, const bool &user_action, const Node_Action::Action &action, const std::shared_ptr<Node> &out, const std::shared_ptr<const Variable_Indices> &variables);
      ZENI_RETE_LINKAGE std::shared_ptr<Node_Action> make_action_retraction(const std::string &name, const bool &user_action, const Node_Action::Action &action, const Node_Action::Action &retraction, const std::shared_ptr<Node> &out, const std::shared_ptr<const Variable_Indices> &variables);
      ZENI_RETE_LINKAGE std::shared_ptr<Node_Existential> make_existential(const std::shared_ptr<Node> &out);
      ZENI_RETE_LINKAGE std::shared_ptr<Node_Join_Existential> make_existential_join(const Variable_Bindings &bindings, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1);
      ZENI_RETE_LINKAGE std::shared_ptr<Node_Filter> make_filter(const WME &wme);
      ZENI_RETE_LINKAGE std::shared_ptr<Node_Join> make_join(const Variable_Bindings &bindings, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1);
      ZENI_RETE_LINKAGE std::shared_ptr<Node_Negation> make_negation(const std::shared_ptr<Node> &out);
      ZENI_RETE_LINKAGE std::shared_ptr<Node_Join_Negation> make_negation_join(const Variable_Bindings &bindings, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1);
      ZENI_RETE_LINKAGE std::shared_ptr<Node_Predicate> make_predicate_vc(const Node_Predicate::Predicate &pred, const Token_Index &lhs_index, const std::shared_ptr<const Symbol> &rhs, const std::shared_ptr<Node> &out);
      ZENI_RETE_LINKAGE std::shared_ptr<Node_Predicate> make_predicate_vv(const Node_Predicate::Predicate &pred, const Token_Index &lhs_index, const Token_Index &rhs_index, const std::shared_ptr<Node> &out);

      ZENI_RETE_LINKAGE std::shared_ptr<Concurrency::Job_Queue> get_Job_Queue() const;
      ZENI_RETE_LINKAGE std::shared_ptr<Concurrency::Thread_Pool> get_Thread_Pool() const { return m_thread_pool; }
      ZENI_RETE_LINKAGE Agenda & get_agenda() { return agenda; }
      ZENI_RETE_LINKAGE std::shared_ptr<Node_Action> get_rule(const std::string &name);
      ZENI_RETE_LINKAGE std::set<std::string> get_rule_names() const;
      ZENI_RETE_LINKAGE int64_t get_rule_name_index() const { return rule_name_index; }
      ZENI_RETE_LINKAGE void set_rule_name_index(const int64_t &rule_name_index_) { rule_name_index = rule_name_index_; }
      ZENI_RETE_LINKAGE Printed_Output get_printed_output() const { return m_printed_output;  }

      ZENI_RETE_LINKAGE void excise_all();
      ZENI_RETE_LINKAGE void excise_filter(const std::shared_ptr<Node_Filter> &filter);
      ZENI_RETE_LINKAGE void excise_rule(const std::string &name, const bool &user_command);
      ZENI_RETE_LINKAGE std::string next_rule_name(const std::string &prefix);
      ZENI_RETE_LINKAGE std::shared_ptr<Node_Action> unname_rule(const std::string &name, const bool &user_command);

      ZENI_RETE_LINKAGE void insert_wme(const std::shared_ptr<const WME> &wme);
      ZENI_RETE_LINKAGE void remove_wme(const std::shared_ptr<const WME> &wme);
      ZENI_RETE_LINKAGE void clear_wmes();

      ZENI_RETE_LINKAGE double rete_cpu_time() const { return m_cpu_time; }
      ZENI_RETE_LINKAGE size_t rete_size() const;
      ZENI_RETE_LINKAGE void rete_print(std::ostream &os) const; ///< Formatted for dot: http://www.graphviz.org/content/dot-language
      ZENI_RETE_LINKAGE void rete_print_rules(std::ostream &os) const;
      ZENI_RETE_LINKAGE void rete_print_firing_counts(std::ostream &os) const;
      ZENI_RETE_LINKAGE void rete_print_matches(std::ostream &os) const;

      template <typename VISITOR>
      VISITOR visit_preorder(VISITOR visitor, const bool &strict) {
        visitor_value = visitor_value != 1 ? 1 : 2;

        for (auto &o : filters)
          visitor = o->visit_preorder(visitor, strict, visitor_value);
        return visitor;
      }

    protected:
      Agenda agenda;

    private:
      void source_rule(const std::shared_ptr<Node_Action> &action, const bool &user_command);

      std::shared_ptr<Concurrency::Thread_Pool> m_thread_pool;

      Node::Filters filters;
      std::unordered_map<std::string, std::shared_ptr<Node_Action>> rules;
      int64_t rule_name_index = 0;
      Working_Memory working_memory;
      intptr_t visitor_value = 0;

      double m_cpu_time = 0.0;
      size_t m_rete_depth = 0;
      std::chrono::high_resolution_clock::time_point m_start;

      // Options

      Node_Sharing m_node_sharing = Node_Sharing::Enabled;
      Printed_Output m_printed_output;
    };

  }

}

#endif
