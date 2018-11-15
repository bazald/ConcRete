#include "Zeni/Rete/Parser/Data.hpp"

#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Filter_1.hpp"
#include "Zeni/Rete/Node_Filter_2.hpp"
#include "Zeni/Rete/Node_Join.hpp"
#include "Zeni/Rete/Node_Join_Existential.hpp"
#include "Zeni/Rete/Node_Join_Negation.hpp"
#include "Zeni/Rete/Variable_Indices.hpp"

#include <sstream>

namespace Zeni::Rete::PEG {

  Symbol_Constant_Generator::Symbol_Constant_Generator(const std::shared_ptr<const Symbol> symbol)
    : m_symbol(symbol)
  {
    assert(!dynamic_cast<const Symbol_Variable *>(m_symbol.get()));
  }

  std::shared_ptr<const Symbol_Generator> Symbol_Constant_Generator::clone(const Symbol_Substitutions &) const {
    return shared_from_this();
  }

  std::shared_ptr<const Symbol> Symbol_Constant_Generator::generate(const Symbol_Substitutions &) const {
    return m_symbol;
  }

  Symbol_Variable_Generator::Symbol_Variable_Generator(const std::shared_ptr<const Symbol_Variable> symbol)
    : m_symbol(symbol)
  {
  }

  std::shared_ptr<const Symbol_Generator> Symbol_Variable_Generator::clone(const Symbol_Substitutions &substitutions) const {
    const auto found = substitutions.find(m_symbol);
    if (found == substitutions.end())
      return shared_from_this();
    else
      return std::make_shared<Symbol_Constant_Generator>(found->second);
  }

  std::shared_ptr<const Symbol> Symbol_Variable_Generator::generate(const Symbol_Substitutions &substitutions) const {
    const auto found = substitutions.find(m_symbol);
    return found == substitutions.end() ? m_symbol : found->second;
  }

  Node_Action_Generator::Node_Action_Generator(const std::shared_ptr<const Symbol_Generator> name_, const std::shared_ptr<const Node_Generator> input_, const Node_Action::Action action_, const Node_Action::Action retraction_)
    : name(name_), input(input_), action(action_), retraction(retraction_)
  {
  }

  std::shared_ptr<const Node_Generator> Node_Action_Generator::clone(const Symbol_Substitutions &substitutions) const {
    const auto name0 = name->clone(substitutions);
    const auto input0 = input->clone(substitutions);
    if (name0 == name && input0 == input)
      return shared_from_this();
    else
      return std::make_shared<Node_Action_Generator>(name0, input0, action, retraction);
  }

  std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>>
    Node_Action_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const Symbol_Substitutions &substitutions) const
  {
    const auto name0 = name->generate(substitutions);
    const auto [input0, key0, variable_indices0] = input->generate(network, job_queue, user_action, substitutions);

    std::ostringstream oss;
    name0->print(oss);

    const auto node = Node_Action::Create(network, job_queue, oss.str(), user_action, key0, input0, variable_indices0, action, retraction);

    return std::make_tuple(node, Node_Key_Null::Create(), dynamic_cast<const Node_Action *>(node.get())->get_variable_indices());
  }

  Node_Filter_Generator::Node_Filter_Generator(const std::shared_ptr<const Symbol_Generator> first_, const std::shared_ptr<const Symbol_Generator> second_, const std::shared_ptr<const Symbol_Generator> third_)
    : first(first_), second(second_), third(third_)
  {
  }

  std::shared_ptr<const Node_Generator> Node_Filter_Generator::clone(const Symbol_Substitutions &substitutions) const {
    const auto symbol0 = first->clone(substitutions);
    const auto symbol1 = second->clone(substitutions);
    const auto symbol2 = third->clone(substitutions);
    if (symbol0 == first && symbol1 == second && symbol2 == third)
      return shared_from_this();
    else
      return std::make_shared<Node_Filter_Generator>(symbol0, symbol1, symbol2);
  }

  std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>>
    Node_Filter_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool, const Symbol_Substitutions &substitutions) const
  {
    const auto symbol0 = first->generate(substitutions);
    const auto symbol1 = second->generate(substitutions);
    const auto symbol2 = third->generate(substitutions);

    const auto var0 = std::dynamic_pointer_cast<const Symbol_Variable>(symbol0);
    const auto var1 = std::dynamic_pointer_cast<const Symbol_Variable>(symbol1);
    const auto var2 = std::dynamic_pointer_cast<const Symbol_Variable>(symbol2);

    std::shared_ptr<Zeni::Rete::Node> node = network;
    auto variable_indices = Variable_Indices::Create();

    std::shared_ptr<const Node_Key> key;
    if (var0) {
      key = Node_Key_Null::Create();
      if (strlen(var0->get_value()) != 0)
        variable_indices->insert(var0->get_value(), Token_Index(0, 0, 0));
    }
    else
      key = Node_Key_Symbol::Create(symbol0);

    if (var1) {
      if (strlen(var1->get_value()) != 0) {
        if (var0 && *var0 == *var1) {
          node = Node_Filter_1::Create(network, job_queue, key);
          key = Node_Key_01::Create();
        }
        else
          variable_indices->insert(var1->get_value(), Token_Index(0, 0, 1));
      }
    }
    else {
      node = Node_Filter_1::Create(network, job_queue, key);
      key = Node_Key_Symbol::Create(symbol1);
    }

    if (var2) {
      if (strlen(var2->get_value()) != 0) {
        if (var0 && *var0 == *var2) {
          node = Node_Filter_2::Create(network, job_queue, key, node);
          key = Node_Key_02::Create();
        }
        else if (var1 && *var1 == *var2) {
          node = Node_Filter_2::Create(network, job_queue, key, node);
          key = Node_Key_12::Create();
        }
        else
          variable_indices->insert(var2->get_value(), Token_Index(0, 0, 2));
      }
    }
    else {
      node = Node_Filter_2::Create(network, job_queue, key, node);
      key = Node_Key_Symbol::Create(symbol2);
    }

    return std::make_tuple(node, key, variable_indices);
  }

  Node_Join_Generator::Node_Join_Generator(const std::shared_ptr<const Node_Generator> left_, const std::shared_ptr<const Node_Generator> right_)
    : left(left_), right(right_)
  {
  }

  std::shared_ptr<const Node_Generator> Node_Join_Generator::clone(const Symbol_Substitutions &substitutions) const {
    const auto node0 = left->clone(substitutions);
    const auto node1 = right->clone(substitutions);
    if (node0 == left && node1 == right)
      return shared_from_this();
    else
      return std::make_shared<Node_Join_Generator>(node0, node1);
  }

  std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>>
    Node_Join_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const Symbol_Substitutions &substitutions) const
  {
    const auto[node0, key0, indices0] = left->generate(network, job_queue, user_action, substitutions);
    const auto[node1, key1, indices1] = right->generate(network, job_queue, user_action, substitutions);

    Variable_Bindings variable_bindings;
    for (auto index_right : indices1->get_indices()) {
      auto index_left = indices0->find_index(index_right.first);
      if (index_left != Token_Index())
        variable_bindings.emplace(index_left, index_right.second);
    }

    const auto node = Node_Join::Create(network, job_queue, key0, key1, node0, node1, std::move(variable_bindings));
    const auto variable_indices = Variable_Indices::Create(node0->get_size(), node0->get_token_size(), *indices0, *indices1);

    return std::make_tuple(node, Node_Key_Null::Create(), variable_indices);
  }

  Node_Join_Existential_Generator::Node_Join_Existential_Generator(const std::shared_ptr<const Node_Generator> left_, const std::shared_ptr<const Node_Generator> right_)
    : left(left_), right(right_)
  {
  }

  std::shared_ptr<const Node_Generator> Node_Join_Existential_Generator::clone(const Symbol_Substitutions &substitutions) const {
    const auto node0 = left->clone(substitutions);
    const auto node1 = right->clone(substitutions);
    if (node0 == left && node1 == right)
      return shared_from_this();
    else
      return std::make_shared<Node_Join_Existential_Generator>(node0, node1);
  }

  std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>>
    Node_Join_Existential_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const Symbol_Substitutions &substitutions) const
  {
    const auto[node0, key0, indices0] = left->generate(network, job_queue, user_action, substitutions);
    const auto[node1, key1, indices1] = right->generate(network, job_queue, user_action, substitutions);

    Variable_Bindings variable_bindings;
    for (auto index_right : indices1->get_indices()) {
      auto index_left = indices0->find_index(index_right.first);
      if (index_left != Token_Index())
        variable_bindings.emplace(index_left, index_right.second);
    }

    const auto node = Node_Join_Existential::Create(network, job_queue, key0, key1, node0, node1, std::move(variable_bindings));
    const auto variable_indices = Variable_Indices::Create(node0->get_size(), node0->get_token_size(), *indices0, *indices1);

    return std::make_tuple(node, Node_Key_Null::Create(), variable_indices);
  }

  Node_Join_Negation_Generator::Node_Join_Negation_Generator(const std::shared_ptr<const Node_Generator> left_, const std::shared_ptr<const Node_Generator> right_)
    : left(left_), right(right_)
  {
  }

  std::shared_ptr<const Node_Generator> Node_Join_Negation_Generator::clone(const Symbol_Substitutions &substitutions) const {
    const auto node0 = left->clone(substitutions);
    const auto node1 = right->clone(substitutions);
    if (node0 == left && node1 == right)
      return shared_from_this();
    else
      return std::make_shared<Node_Join_Negation_Generator>(node0, node1);
  }

  std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>>
    Node_Join_Negation_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const Symbol_Substitutions &substitutions) const
  {
    const auto[node0, key0, indices0] = left->generate(network, job_queue, user_action, substitutions);
    const auto[node1, key1, indices1] = right->generate(network, job_queue, user_action, substitutions);

    Variable_Bindings variable_bindings;
    for (auto index_right : indices1->get_indices()) {
      auto index_left = indices0->find_index(index_right.first);
      if (index_left != Token_Index())
        variable_bindings.emplace(index_left, index_right.second);
    }

    const auto node = Node_Join_Negation::Create(network, job_queue, key0, key1, node0, node1, std::move(variable_bindings));
    const auto variable_indices = Variable_Indices::Create(node0->get_size(), node0->get_token_size(), *indices0, *indices1);

    return std::make_tuple(node, Node_Key_Null::Create(), variable_indices);
  }

  Data::Production::Production(const std::shared_ptr<Network> network_, const std::shared_ptr<Concurrency::Job_Queue> job_queue_)
    : network(network_), job_queue(job_queue_)
  {
    lhs.push(decltype(lhs)::value_type());
  }

  Data::Data(const std::shared_ptr<Network> network_, const std::shared_ptr<Concurrency::Job_Queue> job_queue_, const bool user_command_)
    : network(network_), job_queue(job_queue_), user_command(user_command_)
  {
    productions.push(std::make_shared<Production>(network_, job_queue_, user_command_));
  }

}
