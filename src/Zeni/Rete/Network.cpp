#include "rete_agent.h"

#include <iostream>
#include <map>

namespace Rete {

  Rete_Agent::CPU_Accumulator::CPU_Accumulator(Rete_Agent &agent_)
   : agent(agent_)
  {
    if(++agent.m_rete_depth == 1)
      agent.m_start = std::chrono::high_resolution_clock::now();
  }

  Rete_Agent::CPU_Accumulator::~CPU_Accumulator() {
    if(!--agent.m_rete_depth) {
      using dseconds = std::chrono::duration<double, std::ratio<1,1>>;
      const auto current = std::chrono::high_resolution_clock::now();
      agent.m_cpu_time += std::chrono::duration_cast<dseconds>(current - agent.m_start).count();
    }
  }

  Rete_Agent::Rete_Agent()
  {
  }

  Rete_Agent::~Rete_Agent()
  {
  }

  Rete_Action_Ptr Rete_Agent::make_action(const std::string &name, const bool &user_action, const Rete_Action::Action &action, const Rete_Node_Ptr &out, const Variable_Indices_Ptr_C &variables) {
    CPU_Accumulator cpu_accumulator(*this);

    if(auto existing = Rete_Action::find_existing(action, [](const Rete_Action &, const WME_Token &){}, out))
      return existing;
//      std::cerr << "DEBUG: make_action" << std::endl;
    auto action_fun = std::make_shared<Rete_Action>(name, action, [](const Rete_Action &, const WME_Token &){});
    bind_to_action(*this, action_fun, out, variables);
//      std::cerr << "END: make_action" << std::endl;
    source_rule(action_fun, user_action);

    return action_fun;
  }

  Rete_Action_Ptr Rete_Agent::make_action_retraction(const std::string &name, const bool &user_action, const Rete_Action::Action &action, const Rete_Action::Action &retraction, const Rete_Node_Ptr &out, const Variable_Indices_Ptr_C &variables) {
    CPU_Accumulator cpu_accumulator(*this);

    if(auto existing = Rete_Action::find_existing(action, retraction, out))
      return existing;
    auto action_fun = std::make_shared<Rete_Action>(name, action, retraction);
    bind_to_action(*this, action_fun, out, variables);
    source_rule(action_fun, user_action);
    return action_fun;
  }

  Rete_Existential_Ptr Rete_Agent::make_existential(const Rete_Node_Ptr &out) {
    CPU_Accumulator cpu_accumulator(*this);

    if(auto existing = Rete_Existential::find_existing(out))
      return existing;
    auto existential = std::make_shared<Rete_Existential>();
    bind_to_existential(*this, existential, out);
    return existential;
  }

  Rete_Existential_Join_Ptr Rete_Agent::make_existential_join(const WME_Bindings &bindings, const Rete_Node_Ptr &out0, const Rete_Node_Ptr &out1) {
    CPU_Accumulator cpu_accumulator(*this);

    if(auto existing = Rete_Existential_Join::find_existing(bindings, out0, out1))
      return existing;
    auto existential_join = std::make_shared<Rete_Existential_Join>(bindings);
    bind_to_existential_join(*this, existential_join, out0, out1);
    return existential_join;
  }

  Rete_Filter_Ptr Rete_Agent::make_filter(const WME &wme) {
    CPU_Accumulator cpu_accumulator(*this);

    auto filter = std::make_shared<Rete_Filter>(wme);

    if(!get_Option_Ranged<bool>(Options::get_global(), "rete-disable-node-sharing")) {
      for(auto &existing_filter : filters) {
        if(*existing_filter == *filter)
          return existing_filter;
      }
    }

    bind_to_filter(*this, filter);

    this->filters.push_back(filter);
    for(auto &w : this->working_memory.wmes)
      filter->insert_wme(*this, w);
    return filter;
  }

  Rete_Join_Ptr Rete_Agent::make_join(const WME_Bindings &bindings, const Rete_Node_Ptr &out0, const Rete_Node_Ptr &out1) {
    CPU_Accumulator cpu_accumulator(*this);

    if(auto existing = Rete_Join::find_existing(bindings, out0, out1))
      return existing;
    auto join = std::make_shared<Rete_Join>(bindings);
    bind_to_join(*this, join, out0, out1);
    return join;
  }

  Rete_Negation_Ptr Rete_Agent::make_negation(const Rete_Node_Ptr &out) {
    CPU_Accumulator cpu_accumulator(*this);

    if(auto existing = Rete_Negation::find_existing(out))
      return existing;
    auto negation = std::make_shared<Rete_Negation>();
    bind_to_negation(*this, negation, out);
    return negation;
  }

  Rete_Negation_Join_Ptr Rete_Agent::make_negation_join(const WME_Bindings &bindings, const Rete_Node_Ptr &out0, const Rete_Node_Ptr &out1) {
    CPU_Accumulator cpu_accumulator(*this);

    if(auto existing = Rete_Negation_Join::find_existing(bindings, out0, out1))
      return existing;
    auto negation_join = std::make_shared<Rete_Negation_Join>(bindings);
    bind_to_negation_join(*this, negation_join, out0, out1);
    return negation_join;
  }

  Rete_Predicate_Ptr Rete_Agent::make_predicate_vc(const Rete_Predicate::Predicate &pred, const WME_Token_Index &lhs_index, const Symbol_Ptr_C &rhs, const Rete_Node_Ptr &out) {
    CPU_Accumulator cpu_accumulator(*this);

    if(auto existing = Rete_Predicate::find_existing(pred, lhs_index, rhs, out))
      return existing;
    auto predicate = std::make_shared<Rete_Predicate>(pred, lhs_index, rhs);
    bind_to_predicate(*this, predicate, out);
    return predicate;
  }

  Rete_Predicate_Ptr Rete_Agent::make_predicate_vv(const Rete_Predicate::Predicate &pred, const WME_Token_Index &lhs_index, const WME_Token_Index &rhs_index, const Rete_Node_Ptr &out) {
    CPU_Accumulator cpu_accumulator(*this);

    if(auto existing = Rete_Predicate::find_existing(pred, lhs_index, rhs_index, out))
      return existing;
    auto predicate = std::make_shared<Rete_Predicate>(pred, lhs_index, rhs_index);
    bind_to_predicate(*this, predicate, out);
    return predicate;
  }

  Rete_Action_Ptr Rete_Agent::get_rule(const std::string &name) {
    const auto found = rules.find(name);
    if(found != rules.end())
      return found->second;
    return Rete_Action_Ptr();
  }

  std::set<std::string> Rete_Agent::get_rule_names() const {
    std::set<std::string> rv;
    for(auto rule : rules)
      rv.insert(rule.first);
    return rv;
  }

  void Rete_Agent::excise_all() {
    Agenda::Locker locker(agenda);

    CPU_Accumulator cpu_accumulator(*this);

    filters.clear();
    rules.clear();
  }

  void Rete_Agent::excise_filter(const Rete_Filter_Ptr &filter) {
    CPU_Accumulator cpu_accumulator(*this);

    const auto found = std::find(filters.begin(), filters.end(), filter);
    if(found != filters.end())
      filters.erase(found);
  }

  void Rete_Agent::excise_rule(const std::string &name, const bool &user_command) {
    CPU_Accumulator cpu_accumulator(*this);

    auto found = rules.find(name);
    if(found == rules.end()) {
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
      action->destroy(*this);
      if(user_command)
        std::cerr << '#';
    }
  }

  std::string Rete_Agent::next_rule_name(const std::string &prefix) {
    std::ostringstream oss;
    do {
      oss.str("");
      oss << prefix << ++rule_name_index;
    } while(rules.find(oss.str()) != rules.end());
    return oss.str();
  }

  Rete_Action_Ptr Rete_Agent::unname_rule(const std::string &name, const bool &user_command) {
    CPU_Accumulator cpu_accumulator(*this);

    Rete_Action_Ptr ptr;
    auto found = rules.find(name);
    if(found != rules.end()) {
      ptr = found->second;
      rules.erase(found);
      if(user_command)
        std::cerr << '#';
    }
    return ptr;
  }

  void Rete_Agent::insert_wme(const WME_Ptr_C &wme) {
    ///< Too slow!
//    CPU_Accumulator cpu_accumulator(*this);

    if(working_memory.wmes.find(wme) != working_memory.wmes.end()) {
#ifdef DEBUG_OUTPUT
      std::cerr << "rete.already_inserted" << *wme << std::endl;
#endif
      return;
    }

    //const auto wme_clone = std::make_shared<WME>(Symbol_Ptr_C(wme->symbols[0]->clone()), Symbol_Ptr_C(wme->symbols[1]->clone()), Symbol_Ptr_C(wme->symbols[2]->clone()));

    Agenda::Locker locker(agenda);
    working_memory.wmes.insert(wme);
#ifdef DEBUG_OUTPUT
    std::cerr << "rete.insert" << *wme << std::endl;
#endif
    for(auto &filter : filters)
      filter->insert_wme(*this, wme);
  }

  void Rete_Agent::remove_wme(const WME_Ptr_C &wme) {
    ///< Too slow!
//    CPU_Accumulator cpu_accumulator(*this);

    auto found = working_memory.wmes.find(wme);

    if(found == working_memory.wmes.end()) {
#ifdef DEBUG_OUTPUT
      std::cerr << "rete.already_removed" << *wme << std::endl;
#endif
      return;
    }

    Agenda::Locker locker(agenda);
    working_memory.wmes.erase(found);
#ifdef DEBUG_OUTPUT
    std::cerr << "rete.remove" << *wme << std::endl;
#endif
    for(auto &filter : filters)
      filter->remove_wme(*this, wme);
  }

  void Rete_Agent::clear_wmes() {
    Agenda::Locker locker(agenda);

    CPU_Accumulator cpu_accumulator(*this);

    for(auto &wme : working_memory.wmes) {
      for(auto &filter : filters)
        filter->remove_wme(*this, wme);
    }
    working_memory.wmes.clear();
  }

  size_t Rete_Agent::rete_size() const {
    size_t size = 0;
    std::function<void (const Rete_Node &)> visitor = [&size](const Rete_Node &
#ifndef NDEBUG
                                                              node
#endif
                                                              ){
#ifndef NDEBUG
      if(dynamic_cast<const Rete_Action *>(&node))
        assert(node.get_outputs_all().empty());
      else
        assert(!node.get_outputs_all().empty());
#endif
      ++size;
    };
    const_cast<Rete_Agent *>(this)->visit_preorder(visitor, true);
    return size;
  }

  void Rete_Agent::rete_print(std::ostream &os) const {
    os << "digraph Rete {" << std::endl;

    std::map<Rete_Node_Ptr_C, std::map<int64_t, std::list<Rete_Node_Ptr_C>>> clusters;
    std::map<Rete_Node_Ptr_C, std::list<Rete_Node_Ptr_C>> cluster_children;

    std::function<void (Rete_Node &)> visitor = [&os, &clusters](Rete_Node &node) {
      node.print_details(os);
      if(node.data) {
        clusters[node.data->cluster_root_ancestor()];
      }
    };
    const_cast<Rete_Agent *>(this)->visit_preorder(visitor, false);

    visitor = [&clusters, &cluster_children](Rete_Node &node) {
      if(dynamic_cast<Rete_Filter *>(&node))
        return;
      const auto found = clusters.find(node.shared());
      if(found == clusters.end()) {
        int64_t i = 1;
        for(auto ancestor = node.parent_left(); !dynamic_cast<Rete_Filter *>(ancestor.get()); ancestor = ancestor->parent_left(), ++i) {
          const auto found2 = clusters.find(ancestor);
          if(found2 != clusters.end()) {
            found2->second[i].push_back(node.shared());
            break;
          }
        }
      }
      else {
        found->second[0].push_back(node.shared());
        for(auto ancestor = node.parent_left(); !dynamic_cast<Rete_Filter *>(ancestor.get()); ancestor = ancestor->parent_left()) {
          const auto found2 = clusters.find(ancestor);
          if(found2 != clusters.end()) {
            cluster_children[found2->first].push_back(found->first);
            break;
          }
        }
      }

    };
    const_cast<Rete_Agent *>(this)->visit_preorder(visitor, true);

    if(!filters.empty()) {
      os << "  { rank=source;";
      for(const auto &filter : filters)
        os << ' ' << intptr_t(filter.get());
      os << " }" << std::endl;
    }

    for(const auto &cluster : clusters) {
      os << "  subgraph cluster" << intptr_t(cluster.first.get()) << " {" << std::endl;
      for(const auto &rank_cluster : cluster.second) {
        os << "    { rank=same;";
        for(const auto &node : rank_cluster.second)
          os << ' ' << intptr_t(node.get());
        os << " }" << std::endl;
      }
      os << "  }" << std::endl;
    }

    ///// Force ranks to be top to bottom
    for(const auto &cluster : clusters) {
      const auto found = cluster_children.find(cluster.first);
      if(found != cluster_children.end()) {
        for(const auto &src : cluster.second.rbegin()->second) {
          for(const auto dest : found->second) {
            for(const auto &dest2 : clusters[dest].begin()->second)
              os << "  " << intptr_t(src.get()) << " -> " << intptr_t(dest2.get()) << " [style=\"invis\"]" << std::endl;
          }
        }
      }
    }

    os << "}" << std::endl;
  }

  void Rete_Agent::rete_print_rules(std::ostream &os) const {
    std::multimap<int64_t, Rete_Action_Ptr_C> ordered_rules;
    for(auto it = rules.begin(), iend = rules.end(); it != iend; ++it)
      ordered_rules.insert(std::make_pair(it->second->data? it->second->data->rank() : 0, it->second));

    auto it = ordered_rules.begin();
    const auto iend = ordered_rules.end();

    if(it == iend)
      return;
    it->second->print_rule(os);
    ++it;

    for(; it != iend; ++it) {
      os << std::endl;
      it->second->print_rule(os);
    }
  }

  void Rete_Agent::rete_print_firing_counts(std::ostream &os) const {
    for(auto it = rules.begin(), iend = rules.end(); it != iend; ++it)
      os << it->first << ':' << it->second->parent_left()->get_output_tokens().size() << std::endl;
  }

  void Rete_Agent::rete_print_matches(std::ostream &os) const {
    for(auto it = rules.begin(), iend = rules.end(); it != iend; ++it) {
      os << it->first << " matches:" << std::endl;
      const auto tokens = it->second->parent_left()->get_output_tokens();
      for(const auto &token : tokens)
        os << "  " << *token << std::endl;
    }
  }

  void Rete_Agent::source_rule(const Rete_Action_Ptr &action, const bool &user_command) {
    CPU_Accumulator cpu_accumulator(*this);

    const auto output = dynamic_cast<const Option_Itemized &>(Options::get_global()["output"]).get_value();

    auto found = rules.find(action->get_name());
    if(found == rules.end()) {
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
      found->second->destroy(*this);
      if(user_command && output != "null")
        std::cerr << '#';
      found->second = action;
    }
    if(user_command && output != "null")
      std::cerr << '*';
  }

}
