#ifndef ZENI_RETE_PARSER_DATA_HPP
#define ZENI_RETE_PARSER_DATA_HPP

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Node_Action.hpp"
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

  typedef std::unordered_map<std::shared_ptr<const Symbol_Variable>, std::shared_ptr<const Symbol>> Symbol_Substitutions;

  class Symbol_Generator : public std::enable_shared_from_this<Symbol_Generator> {
    Symbol_Generator(const Symbol_Generator &) = delete;
    Symbol_Generator & operator=(const Symbol_Generator &) = delete;

  public:
    Symbol_Generator() {}

    virtual ~Symbol_Generator() {}

    virtual std::shared_ptr<const Symbol_Generator> clone(const Symbol_Substitutions &substitutions) const = 0;

    virtual std::shared_ptr<const Symbol> generate(const Symbol_Substitutions &substitutions) const = 0;
  };

  class Symbol_Constant_Generator : public Symbol_Generator {
    Symbol_Constant_Generator(const Symbol_Constant_Generator &) = delete;
    Symbol_Constant_Generator & operator=(const Symbol_Constant_Generator &) = delete;

  public:
    Symbol_Constant_Generator(const std::shared_ptr<const Symbol> symbol);

    std::shared_ptr<const Symbol_Generator> clone(const Symbol_Substitutions &substitutions) const override;

    std::shared_ptr<const Symbol> generate(const Symbol_Substitutions &substitutions) const override;

    const std::shared_ptr<const Symbol> m_symbol;
  };

  class Symbol_Variable_Generator : public Symbol_Generator {
    Symbol_Variable_Generator(const Symbol_Variable_Generator &) = delete;
    Symbol_Variable_Generator & operator=(const Symbol_Variable_Generator &) = delete;

  public:
    Symbol_Variable_Generator(const std::shared_ptr<const Symbol_Variable> symbol);

    std::shared_ptr<const Symbol_Generator> clone(const Symbol_Substitutions &substitutions) const override;

    std::shared_ptr<const Symbol> generate(const Symbol_Substitutions &substitutions) const override;

    const std::shared_ptr<const Symbol_Variable> m_symbol;
  };

  class Node_Generator : public std::enable_shared_from_this<Node_Generator> {
    Node_Generator(const Node_Generator &) = delete;
    Node_Generator & operator=(const Node_Generator &) = delete;

  public:
    Node_Generator() {}

    virtual ~Node_Generator() {}

    virtual std::shared_ptr<const Node_Generator> clone(const Symbol_Substitutions &substitutions) const = 0;

    virtual std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>> generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const Symbol_Substitutions &substitutions) const = 0;
  };

  class Node_Action_Generator : public Node_Generator {
    Node_Action_Generator(const Node_Action_Generator &) = delete;
    Node_Action_Generator & operator=(const Node_Action_Generator &) = delete;

  public:
    Node_Action_Generator(const std::shared_ptr<const Symbol_Generator> name, const std::shared_ptr<const Node_Generator> input, const Node_Action::Action action, const Node_Action::Action retraction);

    std::shared_ptr<const Node_Generator> clone(const Symbol_Substitutions &substitutions) const override;

    std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>> generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const Symbol_Substitutions &substitutions) const override;

    const std::shared_ptr<const Symbol_Generator> name;
    const std::shared_ptr<const Node_Generator> input;

    const Node_Action::Action action;
    const Node_Action::Action retraction;
  };

  class Node_Filter_Generator : public Node_Generator {
    Node_Filter_Generator(const Node_Filter_Generator &) = delete;
    Node_Filter_Generator & operator=(const Node_Filter_Generator &) = delete;

  public:
    Node_Filter_Generator(const std::shared_ptr<const Symbol_Generator> first, const std::shared_ptr<const Symbol_Generator> second, const std::shared_ptr<const Symbol_Generator> third);

    std::shared_ptr<const Node_Generator> clone(const Symbol_Substitutions &substitutions) const override;

    std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>> generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const Symbol_Substitutions &substitutions) const override;

    const std::shared_ptr<const Symbol_Generator> first;
    const std::shared_ptr<const Symbol_Generator> second;
    const std::shared_ptr<const Symbol_Generator> third;
  };

  class Node_Join_Generator : public Node_Generator {
    Node_Join_Generator(const Node_Join_Generator &) = delete;
    Node_Join_Generator & operator=(const Node_Join_Generator &) = delete;

  public:
    Node_Join_Generator(const std::shared_ptr<const Node_Generator> left, const std::shared_ptr<const Node_Generator> right);

    std::shared_ptr<const Node_Generator> clone(const Symbol_Substitutions &substitutions) const override;

    std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>> generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const Symbol_Substitutions &substitutions) const override;

    const std::shared_ptr<const Node_Generator> left;
    const std::shared_ptr<const Node_Generator> right;
  };

  class Node_Join_Existential_Generator : public Node_Generator {
    Node_Join_Existential_Generator(const Node_Join_Existential_Generator &) = delete;
    Node_Join_Existential_Generator & operator=(const Node_Join_Existential_Generator &) = delete;

  public:
    Node_Join_Existential_Generator(const std::shared_ptr<const Node_Generator> left, const std::shared_ptr<const Node_Generator> right);

    std::shared_ptr<const Node_Generator> clone(const Symbol_Substitutions &substitutions) const override;

    std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>> generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const Symbol_Substitutions &substitutions) const override;

    const std::shared_ptr<const Node_Generator> left;
    const std::shared_ptr<const Node_Generator> right;
  };

  class Node_Join_Negation_Generator : public Node_Generator {
    Node_Join_Negation_Generator(const Node_Join_Negation_Generator &) = delete;
    Node_Join_Negation_Generator & operator=(const Node_Join_Negation_Generator &) = delete;

  public:
    Node_Join_Negation_Generator(const std::shared_ptr<const Node_Generator> left, const std::shared_ptr<const Node_Generator> right);

    std::shared_ptr<const Node_Generator> clone(const Symbol_Substitutions &substitutions) const override;

    std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>> generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const Symbol_Substitutions &substitutions) const override;

    const std::shared_ptr<const Node_Generator> left;
    const std::shared_ptr<const Node_Generator> right;
  };

  struct Data {
    class Production {
      Production(const Production &) = delete;
      Production & operator=(const Production &) = delete;

    public:
      Production(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue);

      const std::shared_ptr<Network> network;
      const std::shared_ptr<Concurrency::Job_Queue> job_queue;

      std::string rule_name;
      std::stack<std::stack<std::shared_ptr<Node_Generator>>> lhs;

      std::list<Node_Action::Action> actions;
      std::list<Node_Action::Action> retractions;
      std::list<Node_Action::Action> * actions_or_retractions = &actions;

      Symbol_Substitutions substitutions;
    };

    Data(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_command);

    const std::shared_ptr<Network> network;
    const std::shared_ptr<Concurrency::Job_Queue> job_queue;

    const bool user_command;

    std::list<std::shared_ptr<const Symbol_Generator>> symbols;
    std::stack<std::shared_ptr<Production>> productions;
  };

}

#endif
