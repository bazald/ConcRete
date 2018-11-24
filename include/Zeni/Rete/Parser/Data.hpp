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

  class Node_Action_Generator;

  class Symbol_Generator : public std::enable_shared_from_this<Symbol_Generator> {
    Symbol_Generator(const Symbol_Generator &) = delete;
    Symbol_Generator & operator=(const Symbol_Generator &) = delete;

  public:
    Symbol_Generator() {}

    virtual ~Symbol_Generator() {}

    virtual std::shared_ptr<const Symbol_Generator> clone(const std::shared_ptr<const Node_Action::Data> action_data) const = 0;

    virtual std::shared_ptr<const Symbol> generate(const std::shared_ptr<const Node_Action::Data> action_data) const = 0;
  };

  class Symbol_Constant_Generator : public Symbol_Generator {
    Symbol_Constant_Generator(const Symbol_Constant_Generator &) = delete;
    Symbol_Constant_Generator & operator=(const Symbol_Constant_Generator &) = delete;

  public:
    Symbol_Constant_Generator(const std::shared_ptr<const Symbol> symbol);

    std::shared_ptr<const Symbol_Generator> clone(const std::shared_ptr<const Node_Action::Data> action_data) const override;

    std::shared_ptr<const Symbol> generate(const std::shared_ptr<const Node_Action::Data> action_data) const override;

    const std::shared_ptr<const Symbol> symbol;
  };

  class Symbol_Variable_Generator : public Symbol_Generator {
    Symbol_Variable_Generator(const Symbol_Variable_Generator &) = delete;
    Symbol_Variable_Generator & operator=(const Symbol_Variable_Generator &) = delete;

  public:
    Symbol_Variable_Generator(const std::shared_ptr<const Symbol_Variable> symbol);

    std::shared_ptr<const Symbol_Generator> clone(const std::shared_ptr<const Node_Action::Data> action_data) const override;

    std::shared_ptr<const Symbol> generate(const std::shared_ptr<const Node_Action::Data> action_data) const override;

    const std::shared_ptr<const Symbol_Variable> symbol;
  };

  class Action_Generator : public std::enable_shared_from_this<Action_Generator> {
    Action_Generator(const Action_Generator &) = delete;
    Action_Generator & operator=(const Action_Generator &) = delete;

  protected:
    Action_Generator() {}

  public:
    enum Result : uint8_t {
      RESULT_UNTOUCHED = 0x0,
      RESULT_PROVIDED = 0x1,
      RESULT_CONSUMED = 0x2,
      RESULT_CONSUMED_AND_PROVIDED = 0x3};

    virtual ~Action_Generator() {}

    virtual Result result() const { return Result::RESULT_UNTOUCHED; }

    virtual std::shared_ptr<const Action_Generator> clone(const std::shared_ptr<const Node_Action::Data> action_data) const = 0;

    virtual std::shared_ptr<const Node_Action::Action> generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const std::shared_ptr<const Node_Action::Data> action_data) const = 0;
  };

  class Actions_Generator : public Action_Generator {
    Actions_Generator(const Actions_Generator &) = delete;
    Actions_Generator & operator=(const Actions_Generator &) = delete;

  public:
    template <typename Actions>
    Actions_Generator(Actions &&actions) : m_actions(std::forward<Actions>(actions)) {};

    Result result() const override;

    std::shared_ptr<const Action_Generator> clone(const std::shared_ptr<const Node_Action::Data> action_data) const override;

    std::shared_ptr<const Node_Action::Action> generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const std::shared_ptr<const Node_Action::Data> action_data) const override;

  private:
    const std::vector<std::shared_ptr<const Action_Generator>> m_actions;
  };

  class Action_Cbind_Generator : public Action_Generator {
    Action_Cbind_Generator(const Action_Cbind_Generator &) = delete;
    Action_Cbind_Generator & operator=(const Action_Cbind_Generator &) = delete;

  public:
    Action_Cbind_Generator(const std::shared_ptr<const Symbol_Variable> variable);

    Result result() const { return Result::RESULT_CONSUMED; }

    std::shared_ptr<const Action_Generator> clone(const std::shared_ptr<const Node_Action::Data> action_data) const override;

    std::shared_ptr<const Node_Action::Action> generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const std::shared_ptr<const Node_Action::Data> action_data) const override;

  private:
    const std::shared_ptr<const Symbol_Variable> m_variable;
  };

  class Action_Excise_Generator : public Action_Generator {
    Action_Excise_Generator(const Action_Excise_Generator &) = delete;
    Action_Excise_Generator & operator=(const Action_Excise_Generator &) = delete;

  public:
    template <typename Symbols>
    Action_Excise_Generator(Symbols &&symbols) : m_symbols(std::forward<Symbols>(symbols)) {};

    std::shared_ptr<const Action_Generator> clone(const std::shared_ptr<const Node_Action::Data> action_data) const override;

    std::shared_ptr<const Node_Action::Action> generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const std::shared_ptr<const Node_Action::Data> action_data) const override;

  private:
    const std::vector<std::shared_ptr<const Symbol_Generator>> m_symbols;
  };

  class Action_Exit_Generator : public Action_Generator {
    Action_Exit_Generator(const Action_Exit_Generator &) = delete;
    Action_Exit_Generator & operator=(const Action_Exit_Generator &) = delete;

  protected:
    Action_Exit_Generator() {}

  public:
    static std::shared_ptr<const Action_Exit_Generator> Create();

    std::shared_ptr<const Action_Generator> clone(const std::shared_ptr<const Node_Action::Data> action_data) const override;

    std::shared_ptr<const Node_Action::Action> generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const std::shared_ptr<const Node_Action::Data> action_data) const override;
  };

  class Action_Genatom_Generator : public Action_Generator {
    Action_Genatom_Generator(const Action_Genatom_Generator &) = delete;
    Action_Genatom_Generator & operator=(const Action_Genatom_Generator &) = delete;

  protected:
    Action_Genatom_Generator() {}

  public:
    static std::shared_ptr<const Action_Genatom_Generator> Create();

    Result result() const { return Result::RESULT_PROVIDED; }

    std::shared_ptr<const Action_Generator> clone(const std::shared_ptr<const Node_Action::Data> action_data) const override;

    std::shared_ptr<const Node_Action::Action> generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const std::shared_ptr<const Node_Action::Data> action_data) const override;
  };

  class Action_Make_Generator : public Action_Generator {
    Action_Make_Generator(const Action_Make_Generator &) = delete;
    Action_Make_Generator & operator=(const Action_Make_Generator &) = delete;

  public:
    template <typename Symbols>
    Action_Make_Generator(Symbols &&symbols) : m_symbols(std::forward<Symbols>(symbols)) {};

    std::shared_ptr<const Action_Generator> clone(const std::shared_ptr<const Node_Action::Data> action_data) const override;

    std::shared_ptr<const Node_Action::Action> generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const std::shared_ptr<const Node_Action::Data> action_data) const override;

  private:
    const std::vector<std::shared_ptr<const Symbol_Generator>> m_symbols;
  };

  class Action_Production_Generator : public Action_Generator {
    Action_Production_Generator(const Action_Production_Generator &) = delete;
    Action_Production_Generator & operator=(const Action_Production_Generator &) = delete;

  public:
    Action_Production_Generator(const std::shared_ptr<const Node_Action_Generator> action);

    std::shared_ptr<const Action_Generator> clone(const std::shared_ptr<const Node_Action::Data> action_data) const override;

    std::shared_ptr<const Node_Action::Action> generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const std::shared_ptr<const Node_Action::Data> action_data) const override;

  private:
    const std::shared_ptr<const Node_Action_Generator> m_action;
  };

  class Action_Write_Generator : public Action_Generator {
    Action_Write_Generator(const Action_Write_Generator &) = delete;
    Action_Write_Generator & operator=(const Action_Write_Generator &) = delete;

  public:
    Action_Write_Generator(const std::vector<std::shared_ptr<const Symbol_Generator>> &symbols);

    std::shared_ptr<const Action_Generator> clone(const std::shared_ptr<const Node_Action::Data> action_data) const override;

    std::shared_ptr<const Node_Action::Action> generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const std::shared_ptr<const Node_Action::Data> action_data) const override;

  private:
    std::vector<std::shared_ptr<const Symbol_Generator>> m_symbols;
  };

  class Node_Generator : public std::enable_shared_from_this<Node_Generator> {
    Node_Generator(const Node_Generator &) = delete;
    Node_Generator & operator=(const Node_Generator &) = delete;

  public:
    Node_Generator() {}

    virtual ~Node_Generator() {}

    virtual std::shared_ptr<const Node_Generator> clone(const std::shared_ptr<const Node_Action::Data> action_data) const = 0;

    virtual std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>> generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const std::shared_ptr<const Node_Action::Data> action_data) const = 0;
  };

  class Node_Action_Generator : public Node_Generator {
    Node_Action_Generator(const Node_Action_Generator &) = delete;
    Node_Action_Generator & operator=(const Node_Action_Generator &) = delete;

  public:
    Node_Action_Generator(const std::shared_ptr<const Symbol_Generator> name, const std::shared_ptr<const Node_Generator> input, const std::shared_ptr<const Action_Generator> action, const std::shared_ptr<const Action_Generator> retraction);

    std::shared_ptr<const Node_Generator> clone(const std::shared_ptr<const Node_Action::Data> action_data) const override;

    std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>> generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const std::shared_ptr<const Node_Action::Data> action_data) const override;

    const std::shared_ptr<const Symbol_Generator> name;
    const std::shared_ptr<const Node_Generator> input;

    const std::shared_ptr<const Action_Generator> action;
    const std::shared_ptr<const Action_Generator> retraction;
  };

  class Node_Filter_Generator : public Node_Generator {
    Node_Filter_Generator(const Node_Filter_Generator &) = delete;
    Node_Filter_Generator & operator=(const Node_Filter_Generator &) = delete;

  public:
    template <typename First, typename Second, typename Third>
    Node_Filter_Generator(First &&first_, Second &&second_, Third &&third_) : first(std::forward<First>(first_)), second(std::forward<Second>(second_)), third(std::forward<Third>(third_)) {}

    std::shared_ptr<const Node_Generator> clone(const std::shared_ptr<const Node_Action::Data> action_data) const override;

    std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>> generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const std::shared_ptr<const Node_Action::Data> action_data) const override;

    const std::vector<std::shared_ptr<const Symbol_Generator>> first;
    const std::vector<std::shared_ptr<const Symbol_Generator>> second;
    const std::vector<std::shared_ptr<const Symbol_Generator>> third;
  };

  class Node_Join_Generator : public Node_Generator {
    Node_Join_Generator(const Node_Join_Generator &) = delete;
    Node_Join_Generator & operator=(const Node_Join_Generator &) = delete;

  public:
    Node_Join_Generator(const std::shared_ptr<const Node_Generator> left, const std::shared_ptr<const Node_Generator> right);

    std::shared_ptr<const Node_Generator> clone(const std::shared_ptr<const Node_Action::Data> action_data) const override;

    std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>> generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const std::shared_ptr<const Node_Action::Data> action_data) const override;

    const std::shared_ptr<const Node_Generator> left;
    const std::shared_ptr<const Node_Generator> right;
  };

  class Node_Join_Existential_Generator : public Node_Generator {
    Node_Join_Existential_Generator(const Node_Join_Existential_Generator &) = delete;
    Node_Join_Existential_Generator & operator=(const Node_Join_Existential_Generator &) = delete;

  public:
    Node_Join_Existential_Generator(const std::shared_ptr<const Node_Generator> left, const std::shared_ptr<const Node_Generator> right);

    std::shared_ptr<const Node_Generator> clone(const std::shared_ptr<const Node_Action::Data> action_data) const override;

    std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>> generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const std::shared_ptr<const Node_Action::Data> action_data) const override;

    const std::shared_ptr<const Node_Generator> left;
    const std::shared_ptr<const Node_Generator> right;
  };

  class Node_Join_Negation_Generator : public Node_Generator {
    Node_Join_Negation_Generator(const Node_Join_Negation_Generator &) = delete;
    Node_Join_Negation_Generator & operator=(const Node_Join_Negation_Generator &) = delete;

  public:
    Node_Join_Negation_Generator(const std::shared_ptr<const Node_Generator> left, const std::shared_ptr<const Node_Generator> right);

    std::shared_ptr<const Node_Generator> clone(const std::shared_ptr<const Node_Action::Data> action_data) const override;

    std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>> generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const std::shared_ptr<const Node_Action::Data> action_data) const override;

    const std::shared_ptr<const Node_Generator> left;
    const std::shared_ptr<const Node_Generator> right;
  };

  struct Data {
    class Production {
      Production(const Production &) = delete;
      Production & operator=(const Production &) = delete;

    public:
      Production(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const std::unordered_set<std::string> &lhs_variables);

      const std::shared_ptr<Network> network;
      const std::shared_ptr<Concurrency::Job_Queue> job_queue;

      bool user_action;

      enum class Phase {PHASE_INIT, PHASE_LHS, PHASE_ACTIONS, PHASE_RETRACTIONS};

      Phase phase = Phase::PHASE_INIT;
      bool result_available = false;

      std::shared_ptr<const Symbol_Generator> rule_name;
      std::stack<std::stack<std::shared_ptr<Node_Generator>>> lhs;
      std::unordered_set<std::string> lhs_variables;

      std::vector<std::shared_ptr<const Action_Generator>> actions;
      std::vector<std::shared_ptr<const Action_Generator>> retractions;
      std::vector<std::shared_ptr<const Action_Generator>> * actions_or_retractions = &actions;
    };

    Data(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action);

    const std::shared_ptr<Network> network;
    const std::shared_ptr<Concurrency::Job_Queue> job_queue;

    std::stack<std::vector<std::shared_ptr<const Symbol_Generator>>> symbols;
    std::stack<std::shared_ptr<Production>> productions;
  };

}

#endif
