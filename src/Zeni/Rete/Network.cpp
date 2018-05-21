#include "Zeni/Rete/network.hpp"

#include "Zeni/Concurrency/Thread_Pool.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Node_Existential.hpp"
#include "Zeni/Rete/Node_Filter.hpp"
#include "Zeni/Rete/Node_Join.hpp"
#include "Zeni/Rete/Node_Join_Existential.hpp"
#include "Zeni/Rete/Node_Join_Negation.hpp"
#include "Zeni/Rete/Node_Negation.hpp"
#include "Zeni/Rete/Node_Predicate.hpp"

#include <iostream>
#include <map>
#include <sstream>

namespace Zeni {

  namespace Rete {

    Network::CPU_Accumulator::CPU_Accumulator(const std::shared_ptr<Network> &network_)
      : network(network_)
    {
      if (++network->m_rete_depth == 1)
        network->m_start = std::chrono::high_resolution_clock::now();
    }

    Network::CPU_Accumulator::~CPU_Accumulator() {
      if (!--network->m_rete_depth) {
        using dseconds = std::chrono::duration<double, std::ratio<1, 1>>;
        const auto current = std::chrono::high_resolution_clock::now();
        network->m_cpu_time += std::chrono::duration_cast<dseconds>(current - network->m_start).count();
      }
    }

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
      m_thread_pool.reset();
    }

    Network::~Network()
    {
    }

    std::shared_ptr<Concurrency::Job_Queue> Network::get_Job_Queue() const {
      return m_thread_pool->get_Job_Queue();
    }

    std::shared_ptr<Node_Action> Network::get_rule(const std::string &name) {
      const auto found = rules.find(name);
      if (found != rules.end())
        return found->second;
      return std::shared_ptr<Node_Action>();
    }

    std::set<std::string> Network::get_rule_names() const {
      std::set<std::string> rv;
      for (auto rule : rules)
        rv.insert(rule.first);
      return rv;
    }

    void Network::excise_all() {
      Agenda::Locker locker(agenda);

      CPU_Accumulator cpu_accumulator(shared_from_this());

      filters.clear();
      rules.clear();
    }

    void Network::source_filter(const std::shared_ptr<Node_Filter> &filter) {
      CPU_Accumulator cpu_accumulator(shared_from_this());

      filters.push_back(filter);
      const auto sft = shared_from_this();
      for (auto &w : working_memory.get_wmes())
        filter->insert_wme(sft, w);
    }

    void Network::excise_filter(const std::shared_ptr<Node_Filter> &filter) {
      CPU_Accumulator cpu_accumulator(shared_from_this());

      const auto found = std::find(filters.begin(), filters.end(), filter);
      if (found != filters.end())
        filters.erase(found);
    }

    void Network::excise_rule(const std::string &name, const bool &user_command) {
      CPU_Accumulator cpu_accumulator(shared_from_this());

      auto found = rules.find(name);
      if (found == rules.end()) {
        //#ifndef NDEBUG
        //      std::cerr << "Rule '" << name << "' not found." << std::endl;
        //#endif
      }
      else {
        //#ifndef NDEBUG
        //      std::cerr << "Rule '" << name << "' excised." << std::endl;
        //#endif
        auto action = found->second;
        rules.erase(found);
        action->Destroy(shared_from_this());
        if (user_command)
          std::cerr << '#';
      }
    }

    std::string Network::next_rule_name(const std::string &prefix) {
      std::ostringstream oss;
      do {
        oss.str("");
        oss << prefix << ++rule_name_index;
      } while (rules.find(oss.str()) != rules.end());
      return oss.str();
    }

    std::shared_ptr<Node_Action> Network::unname_rule(const std::string &name, const bool &user_command) {
      CPU_Accumulator cpu_accumulator(shared_from_this());

      std::shared_ptr<Node_Action> ptr;
      auto found = rules.find(name);
      if (found != rules.end()) {
        ptr = found->second;
        rules.erase(found);
        if (user_command)
          std::cerr << '#';
      }
      return ptr;
    }

    void Network::insert_wme(const std::shared_ptr<const WME> &wme) {
      ///< Too slow!
  //    CPU_Accumulator cpu_accumulator(*this);

      if (working_memory.get_wmes().find(wme) != working_memory.get_wmes().end()) {
#ifdef DEBUG_OUTPUT
        std::cerr << "rete.already_inserted" << *wme << std::endl;
#endif
        return;
      }

      //const auto wme_clone = std::make_shared<WME>(std::shared_ptr<const Symbol>(wme->symbols[0]->clone()), std::shared_ptr<const Symbol>(wme->symbols[1]->clone()), std::shared_ptr<const Symbol>(wme->symbols[2]->clone()));

      Agenda::Locker locker(agenda);
      working_memory.get_wmes().insert(wme);
#ifdef DEBUG_OUTPUT
      std::cerr << "rete.insert" << *wme << std::endl;
#endif
      for (auto &filter : filters)
        filter->insert_wme(shared_from_this(), wme);
    }

    void Network::remove_wme(const std::shared_ptr<const WME> &wme) {
      ///< Too slow!
  //    CPU_Accumulator cpu_accumulator(*this);

      auto found = working_memory.get_wmes().find(wme);

      if (found == working_memory.get_wmes().end()) {
#ifdef DEBUG_OUTPUT
        std::cerr << "rete.already_removed" << *wme << std::endl;
#endif
        return;
      }

      Agenda::Locker locker(agenda);
      working_memory.get_wmes().erase(found);
#ifdef DEBUG_OUTPUT
      std::cerr << "rete.remove" << *wme << std::endl;
#endif
      for (auto &filter : filters)
        filter->remove_wme(shared_from_this(), wme);
    }

    void Network::clear_wmes() {
      Agenda::Locker locker(agenda);

      CPU_Accumulator cpu_accumulator(shared_from_this());

      for (auto &wme : working_memory.get_wmes()) {
        for (auto &filter : filters)
          filter->remove_wme(shared_from_this(), wme);
      }
      working_memory.get_wmes().clear();
    }

    size_t Network::rete_size() const {
      size_t size = 0;
      std::function<void(const std::shared_ptr<Node> &)> visitor = [&size](const std::shared_ptr<Node> &
#ifndef NDEBUG
        node
#endif
        ) {
#ifndef NDEBUG
        if (std::dynamic_pointer_cast<const Node_Action>(node))
          assert(node->get_outputs_all().empty());
        else
          assert(!node->get_outputs_all().empty());
#endif
        ++size;
      };
      const_cast<Network *>(this)->visit_preorder(visitor, true);
      return size;
    }

    void Network::rete_print(std::ostream &os) const {
      os << "digraph Rete {" << std::endl;

      std::map<std::shared_ptr<const Node>, std::map<int64_t, std::list<std::shared_ptr<const Node>>>> clusters;
      std::map<std::shared_ptr<const Node>, std::list<std::shared_ptr<const Node>>> cluster_children;

      std::function<void(const std::shared_ptr<Node> &)> visitor = [&os, &clusters](const std::shared_ptr<Node> &node) {
        node->print_details(os);
        if (node->custom_data) {
          clusters[node->custom_data->cluster_root_ancestor()];
        }
      };
      const_cast<Network *>(this)->visit_preorder(visitor, false);

      visitor = [&clusters, &cluster_children](const std::shared_ptr<Node> &node) {
        if (std::dynamic_pointer_cast<Node_Filter>(node))
          return;
        const auto found = clusters.find(node);
        if (found == clusters.end()) {
          int64_t i = 1;
          for (auto ancestor = node->parent_left(); !dynamic_cast<Node_Filter *>(ancestor.get()); ancestor = ancestor->parent_left(), ++i) {
            const auto found2 = clusters.find(ancestor);
            if (found2 != clusters.end()) {
              found2->second[i].push_back(node);
              break;
            }
          }
        }
        else {
          found->second[0].push_back(node);
          for (auto ancestor = node->parent_left(); !dynamic_cast<Node_Filter *>(ancestor.get()); ancestor = ancestor->parent_left()) {
            const auto found2 = clusters.find(ancestor);
            if (found2 != clusters.end()) {
              cluster_children[found2->first].push_back(found->first);
              break;
            }
          }
        }

      };
      const_cast<Network *>(this)->visit_preorder(visitor, true);

      if (!filters.empty()) {
        os << "  { rank=source;";
        for (const auto &filter : filters)
          os << ' ' << intptr_t(filter.get());
        os << " }" << std::endl;
      }

      for (const auto &cluster : clusters) {
        os << "  subgraph cluster" << intptr_t(cluster.first.get()) << " {" << std::endl;
        for (const auto &rank_cluster : cluster.second) {
          os << "    { rank=same;";
          for (const auto &node : rank_cluster.second)
            os << ' ' << intptr_t(node.get());
          os << " }" << std::endl;
        }
        os << "  }" << std::endl;
      }

      ///// Force ranks to be top to bottom
      for (const auto &cluster : clusters) {
        const auto found = cluster_children.find(cluster.first);
        if (found != cluster_children.end()) {
          for (const auto &src : cluster.second.rbegin()->second) {
            for (const auto dest : found->second) {
              for (const auto &dest2 : clusters[dest].begin()->second)
                os << "  " << intptr_t(src.get()) << " -> " << intptr_t(dest2.get()) << " [style=\"invis\"]" << std::endl;
            }
          }
        }
      }

      os << "}" << std::endl;
    }

    void Network::rete_print_rules(std::ostream &os) const {
      std::multimap<int64_t, std::shared_ptr<const Node_Action>> ordered_rules;
      for (auto it = rules.begin(), iend = rules.end(); it != iend; ++it)
        ordered_rules.insert(std::make_pair(it->second->custom_data ? it->second->custom_data->rank() : 0, it->second));

      auto it = ordered_rules.begin();
      const auto iend = ordered_rules.end();

      if (it == iend)
        return;
      it->second->print_rule(os);
      ++it;

      for (; it != iend; ++it) {
        os << std::endl;
        it->second->print_rule(os);
      }
    }

    void Network::rete_print_firing_counts(std::ostream &os) const {
      for (auto it = rules.begin(), iend = rules.end(); it != iend; ++it)
        os << it->first << ':' << it->second->parent_left()->get_output_tokens().size() << std::endl;
    }

    void Network::rete_print_matches(std::ostream &os) const {
      for (auto it = rules.begin(), iend = rules.end(); it != iend; ++it) {
        os << it->first << " matches:" << std::endl;
        const auto tokens = it->second->parent_left()->get_output_tokens();
        for (const auto &token : tokens)
          os << "  " << *token << std::endl;
      }
    }

    void Network::source_rule(const std::shared_ptr<Node_Action> &action, const bool &user_command) {
      const auto sft = shared_from_this();
      CPU_Accumulator cpu_accumulator(sft);

      auto found = rules.find(action->get_name());
      if (found == rules.end()) {
        //#ifndef NDEBUG
        //      std::cerr << "Rule '" << action->get_name() << "' sourced." << std::endl;
        //#endif
        rules[action->get_name()] = action;
      }
      else {
        //#ifndef NDEBUG
        //      std::cerr << "Rule '" << action->get_name() << "' replaced." << std::endl;
        //#endif
        assert(found->second != action);
        found->second->Destroy(sft);
        if (user_command && m_printed_output != Printed_Output::None)
          std::cerr << '#';
        found->second = action;
      }
      if (user_command && m_printed_output != Printed_Output::None)
        std::cerr << '*';
    }

  }

}
