#ifndef ZENI_RETE_PARSER_DATA_HPP
#define ZENI_RETE_PARSER_DATA_HPP

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Node.hpp"
#include "Zeni/Rete/Node_Key.hpp"
#include "Zeni/Rete/Variable_Indices.hpp"

#include <functional>
#include <list>
#include <memory>
#include <stack>

namespace Zeni::Rete {
  class Network;
  class Node_Action;
  class Token;
}

namespace Zeni::Rete::PEG {

  struct Data {
    class Production {
      Production(const Production &) = delete;
      Production & operator=(const Production &) = delete;

    public:
      Production(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_command);

      ~Production();

      const std::shared_ptr<Network> network;
      const std::shared_ptr<Concurrency::Job_Queue> job_queue;

      const bool user_command;

      std::string rule_name;
      std::stack<std::stack<std::pair<std::pair<std::shared_ptr<Zeni::Rete::Node>, std::shared_ptr<const Node_Key>>, std::shared_ptr<Variable_Indices>>>> lhs;

      typedef std::function<void(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token)> NAA;
      std::list<NAA> actions;
      std::list<NAA> retractions;
      std::list<NAA> * actions_or_retractions = &actions;
    };

    Data(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_command);

    template <typename Join_Type>
    void join_conditions() {
      const auto node_right = productions.top()->lhs.top().top();
      productions.top()->lhs.top().pop();
      const auto node_left = productions.top()->lhs.top().top();
      productions.top()->lhs.top().pop();

      Variable_Bindings variable_bindings;
      for (auto right : node_right.second->get_indices()) {
        auto left = node_left.second->find_index(right.first);
        if (left != Token_Index())
          variable_bindings.emplace(left, right.second);
      }

      const auto node = Join_Type::Create(network, job_queue, node_left.first.second, node_right.first.second, node_left.first.first, node_right.first.first, std::move(variable_bindings));
      const auto variable_indices = Variable_Indices::Create(node_left.first.first->get_size(), node_left.first.first->get_token_size(), *node_left.second, *node_right.second);

      productions.top()->lhs.top().emplace(std::make_pair(node, Node_Key_Null::Create()), variable_indices);
    }

    const std::shared_ptr<Network> network;
    const std::shared_ptr<Concurrency::Job_Queue> job_queue;

    std::list<std::pair<std::string, std::shared_ptr<const Rete::Symbol>>> symbols;
    std::stack<std::shared_ptr<Production>> productions;
  };

}

#endif
