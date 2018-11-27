#include "Zeni/Rete/Parser/Data.hpp"

#include "Zeni/Rete/Internal/Token_Alpha.hpp"
#include "Zeni/Rete/Internal/Token_Beta.hpp"
#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Filter_1.hpp"
#include "Zeni/Rete/Node_Filter_2.hpp"
#include "Zeni/Rete/Node_Join.hpp"
#include "Zeni/Rete/Node_Join_Existential.hpp"
#include "Zeni/Rete/Node_Join_Negation.hpp"
#include "Zeni/Rete/Variable_Indices.hpp"

#include <algorithm>
#include <sstream>

namespace Zeni::Rete::PEG {

  Symbol_Constant_Generator::Symbol_Constant_Generator(const std::shared_ptr<const Symbol> symbol_)
    : symbol(symbol_)
  {
    assert(!dynamic_cast<const Symbol_Variable *>(symbol.get()));
  }

  std::shared_ptr<const Symbol_Generator> Symbol_Constant_Generator::clone(const std::shared_ptr<const Node_Action::Data> action_data) const {
    return shared_from_this();
  }

  std::shared_ptr<const Symbol> Symbol_Constant_Generator::generate(const std::shared_ptr<const Node_Action::Data> action_data) const {
    return symbol;
  }

  Symbol_Variable_Generator::Symbol_Variable_Generator(const std::shared_ptr<const Symbol_Variable> symbol_)
    : symbol(symbol_)
  {
  }

  std::shared_ptr<const Symbol_Generator> Symbol_Variable_Generator::clone(const std::shared_ptr<const Node_Action::Data> action_data) const {
    if (!action_data)
      return shared_from_this();

    const auto found = action_data->variable_indices->find_index(symbol->get_value());
    if (found != Token_Index())
      return std::make_shared<Symbol_Constant_Generator>((*action_data->token)[found]);
    else
      return shared_from_this();
  }

  std::shared_ptr<const Symbol> Symbol_Variable_Generator::generate(const std::shared_ptr<const Node_Action::Data> action_data) const {
    if (!action_data)
      return symbol;

    const auto found = action_data->variable_indices->find_index(symbol->get_value());
    if (found != Token_Index())
      return (*action_data->token)[found];
    else
      return symbol;
  }

  Action_Generator::Result Actions_Generator::result() const {
    uint8_t rv = Result::RESULT_UNTOUCHED;

    for (auto at = m_actions.cbegin(), aend = m_actions.cend(); at != aend; ++at) {
      const auto irv = (*at)->result();
      if (irv == Result::RESULT_UNTOUCHED)
        continue;
      if (irv & Result::RESULT_CONSUMED)
        rv |= Result::RESULT_CONSUMED;
      break;
    }

    for (auto at = m_actions.crbegin(), aend = m_actions.crend(); at != aend; ++at) {
      const auto irv = (*at)->result();
      if (irv == Result::RESULT_UNTOUCHED)
        continue;
      if (irv & Result::RESULT_PROVIDED)
        rv |= Result::RESULT_PROVIDED;
      break;
    }

    return Result(rv);
  }

  std::shared_ptr<const Action_Generator> Actions_Generator::clone(const std::shared_ptr<const Node_Action::Data> action_data) const {
    std::vector<std::shared_ptr<const Action_Generator>> actions;
    actions.reserve(m_actions.size());
    for (const auto action : m_actions)
      actions.push_back(action->clone(action_data));
    for (auto at = actions.cbegin(), mt = m_actions.cbegin(), aend = actions.cend(); at != aend; ++at, ++mt) {
      if (*at != *mt)
        return std::make_shared<Actions_Generator>(actions);
    }
    return shared_from_this();
  }

  std::shared_ptr<const Node_Action::Action> Actions_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool, const std::shared_ptr<const Node_Action::Data> action_data) const {
    std::vector<std::shared_ptr<const Node_Action::Action>> actions;
    actions.reserve(m_actions.size());
    for (const auto action : m_actions)
      actions.push_back(action->generate(network, job_queue, false, action_data));
    
    class Actions : public Node_Action::Action {
      Actions(const Actions &) = delete;
      Actions & operator=(const Actions &) = delete;

    public:
      Actions(const std::vector<std::shared_ptr<const Node_Action::Action>> actions)
        : m_actions(actions)
      {
      }

      void operator()(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Node_Action::Data> action_data) const override {
        for (const auto action : m_actions)
          (*action)(network, job_queue, rete_action, action_data);
      }

    private:
      const std::vector<std::shared_ptr<const Node_Action::Action>> m_actions;
    };

    return std::make_shared<Actions>(actions);
  }

  Action_Cbind_Generator::Action_Cbind_Generator(const std::shared_ptr<const Symbol_Variable> variable)
    : m_variable(variable)
  {
  }

  std::shared_ptr<const Action_Generator> Action_Cbind_Generator::clone(const std::shared_ptr<const Node_Action::Data>) const {
    return shared_from_this();
  }

  std::shared_ptr<const Node_Action::Action> Action_Cbind_Generator::generate(const std::shared_ptr<Network>, const std::shared_ptr<Concurrency::Job_Queue>, const bool, const std::shared_ptr<const Node_Action::Data>) const {
    class Action_Cbind : public Node_Action::Action {
      Action_Cbind(const Action_Cbind &) = delete;
      Action_Cbind & operator=(const Action_Cbind &) = delete;

    public:
      Action_Cbind(const std::shared_ptr<const Symbol_Variable> &variable)
        : m_variable(variable)
      {
      }

      void operator()(const std::shared_ptr<Network>, const std::shared_ptr<Concurrency::Job_Queue>, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Node_Action::Data> action_data) const override {
        const auto token_left = action_data->token;
        const auto token_right = std::make_shared<Token_Alpha>(std::make_shared<WME>(action_data->result, nullptr, nullptr));
        action_data->result = nullptr;
        const auto variable_indices_right = Variable_Indices::Create();
        variable_indices_right->insert(m_variable->get_value(), Token_Index(0, 0, 0));
        action_data->variable_indices = Variable_Indices::Create(rete_action->get_size(), token_left->size(), *action_data->variable_indices, *variable_indices_right);
        action_data->token = std::make_shared<Token_Beta>(token_left, token_right);
      }

    private:
      const std::shared_ptr<const Symbol_Variable> m_variable;
    };

    return std::make_shared<Action_Cbind>(m_variable);
  }

  std::shared_ptr<const Action_Generator> Action_Excise_Generator::clone(const std::shared_ptr<const Node_Action::Data> action_data) const {
    std::vector<std::shared_ptr<const Symbol_Generator>> symbols;
    symbols.reserve(m_symbols.size());
    for (const auto symbol : m_symbols)
      symbols.push_back(symbol->clone(action_data));
    for (auto st = symbols.cbegin(), mt = m_symbols.cbegin(), send = symbols.cend(); st != send; ++st, ++mt) {
      if (*st != *mt)
        return std::make_shared<Action_Excise_Generator>(symbols);
    }
    return shared_from_this();
  }

  std::shared_ptr<const Node_Action::Action> Action_Excise_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const std::shared_ptr<const Node_Action::Data> action_data) const {
    class Get_Symbol : public std::enable_shared_from_this<Get_Symbol> {
    public:
      virtual ~Get_Symbol() {}

      virtual std::shared_ptr<const Symbol_Constant> operator()(const std::shared_ptr<const Node_Action::Data> action_data) const = 0;
    };

    class Get_Symbol_Constant : public Get_Symbol {
    public:
      Get_Symbol_Constant(const std::shared_ptr<const Symbol_Constant> symbol_) : symbol(symbol_) {}

      std::shared_ptr<const Symbol_Constant> operator()(const std::shared_ptr<const Node_Action::Data>) const override {
        return symbol;
      }

      const std::shared_ptr<const Symbol_Constant> symbol;
    };

    class Get_Symbol_Variable : public Get_Symbol {
    public:
      Get_Symbol_Variable(const std::shared_ptr<const Symbol_Variable> variable_) : variable(variable_) {}

      std::shared_ptr<const Symbol_Constant> operator()(const std::shared_ptr<const Node_Action::Data> action_data) const override {
        assert(action_data);
        return std::dynamic_pointer_cast<const Symbol_Constant>((*action_data->token)[action_data->variable_indices->find_index(variable->get_value())]);
      }

      const std::shared_ptr<const Symbol_Variable> variable;
    };

    std::vector<std::shared_ptr<const Get_Symbol>> symbols;
    symbols.reserve(m_symbols.size());
    for (const auto gen_symbol : m_symbols) {
      const auto symbol = gen_symbol->generate(action_data);
      if (const auto variable = std::dynamic_pointer_cast<const Symbol_Variable>(symbol))
        symbols.push_back(std::make_shared<Get_Symbol_Variable>(variable));
      else
        symbols.push_back(std::make_shared<Get_Symbol_Constant>(std::dynamic_pointer_cast<const Symbol_Constant>(symbol)));
    }

    class Action_Excise : public Node_Action::Action {
      Action_Excise(const Action_Excise &) = delete;
      Action_Excise & operator=(const Action_Excise &) = delete;

    public:
      Action_Excise(const std::vector<std::shared_ptr<const Get_Symbol>> &symbols, const bool user_action)
        : m_symbols(symbols), m_user_action(user_action)
      {
      }

      Action_Excise(std::vector<std::shared_ptr<const Get_Symbol>> &&symbols, const bool user_action)
        : m_symbols(std::move(symbols)), m_user_action(user_action)
      {
      }

      void operator()(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Node_Action::Data> action_data) const override {
        for (const auto symbol : m_symbols) {
          std::ostringstream oss;
          oss << *(*symbol)(action_data);
          network->excise_rule(job_queue, oss.str(), m_user_action);
        }
      }

    private:
      const std::vector<std::shared_ptr<const Get_Symbol>> m_symbols;
      const bool m_user_action;
    };

    return std::make_shared<Action_Excise>(symbols, user_action);
  }

  std::shared_ptr<const Action_Excise_All_Generator> Action_Excise_All_Generator::Create() {
    class Friendly_Action_Excise_All_Generator : public Action_Excise_All_Generator {
    public:
      Friendly_Action_Excise_All_Generator() {}
    };

    static const auto action_excise_all_generator = std::make_shared<Friendly_Action_Excise_All_Generator>();

    return action_excise_all_generator;
  }

  std::shared_ptr<const Action_Generator> Action_Excise_All_Generator::clone(const std::shared_ptr<const Node_Action::Data>) const {
    return shared_from_this();
  }

  std::shared_ptr<const Node_Action::Action> Action_Excise_All_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const std::shared_ptr<const Node_Action::Data> action_data) const {
    class Action_All_Excise : public Node_Action::Action {
      Action_All_Excise(const Action_All_Excise &) = delete;
      Action_All_Excise & operator=(const Action_All_Excise &) = delete;

    public:
      Action_All_Excise(const bool user_action)
        : m_user_action(user_action)
      {
      }

      void operator()(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Node_Action::Data> action_data) const override {
        network->excise_all(job_queue, m_user_action);
      }

    private:
      const bool m_user_action;
    };

    return std::make_shared<Action_All_Excise>(user_action);
  }

  std::shared_ptr<const Action_Genatom_Generator> Action_Genatom_Generator::Create() {
    class Friendly_Action_Genatom_Generator : public Action_Genatom_Generator {
    public:
      Friendly_Action_Genatom_Generator() {}
    };

    static const auto action_exit_generator = std::make_shared<Friendly_Action_Genatom_Generator>();

    return action_exit_generator;
  }

  std::shared_ptr<const Action_Generator> Action_Genatom_Generator::clone(const std::shared_ptr<const Node_Action::Data> action_data) const {
    return shared_from_this();
  }

  std::shared_ptr<const Node_Action::Action> Action_Genatom_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool, const std::shared_ptr<const Node_Action::Data> action_data) const {
    class Action_Genatom : public Node_Action::Action {
      Action_Genatom(const Action_Genatom &) = delete;
      Action_Genatom & operator=(const Action_Genatom &) = delete;

      Action_Genatom() {}

    public:
      static std::shared_ptr<const Action_Genatom> Create() {
        class Friendly_Action_Genatom : public Action_Genatom {
        public:
          Friendly_Action_Genatom() {}
        };

        static const auto action_genatom = std::make_shared<Friendly_Action_Genatom>();

        return action_genatom;
      }

      void operator()(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue>, const std::shared_ptr<Node_Action>, const std::shared_ptr<const Node_Action::Data> action_data) const override {
        action_data->result = std::make_shared<Symbol_Constant_String>(network->genatom());
      }
    };

    return Action_Genatom::Create();
  }

  std::shared_ptr<const Action_Exit_Generator> Action_Exit_Generator::Create() {
    class Friendly_Action_Exit_Generator : public Action_Exit_Generator {
    public:
      Friendly_Action_Exit_Generator() {}
    };

    static const auto action_exit_generator = std::make_shared<Friendly_Action_Exit_Generator>();

    return action_exit_generator;
  }

  std::shared_ptr<const Action_Generator> Action_Exit_Generator::clone(const std::shared_ptr<const Node_Action::Data> action_data) const {
    return shared_from_this();
  }

  std::shared_ptr<const Node_Action::Action> Action_Exit_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool, const std::shared_ptr<const Node_Action::Data> action_data) const {
    class Action_Exit : public Node_Action::Action {
      Action_Exit(const Action_Exit &) = delete;
      Action_Exit & operator=(const Action_Exit &) = delete;

      Action_Exit() {}

    public:
      static std::shared_ptr<const Action_Exit> Create() {
        class Friendly_Action_Exit : public Action_Exit {
        public:
          Friendly_Action_Exit() {}
        };

        static const auto action_exit = std::make_shared<Friendly_Action_Exit>();

        return action_exit;
      }

      void operator()(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue>, const std::shared_ptr<Node_Action>, const std::shared_ptr<const Node_Action::Data>) const override {
        network->request_exit();
      }
    };

    return Action_Exit::Create();
  }

  std::shared_ptr<const Action_Generator> Action_Make_Generator::clone(const std::shared_ptr<const Node_Action::Data> action_data) const {
    std::vector<std::shared_ptr<const Symbol_Generator>> symbols;
    symbols.reserve(m_symbols.size());
    for (const auto symbol : m_symbols)
      symbols.push_back(symbol->clone(action_data));
    for (auto st = symbols.cbegin(), mt = m_symbols.cbegin(), send = symbols.cend(); st != send; ++st, ++mt) {
      if (*st != *mt)
        return std::make_shared<Action_Make_Generator>(symbols);
    }
    return shared_from_this();
  }

  std::shared_ptr<const Node_Action::Action> Action_Make_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool, const std::shared_ptr<const Node_Action::Data> action_data) const {
    class Get_Symbol : public std::enable_shared_from_this<Get_Symbol> {
    public:
      virtual ~Get_Symbol() {}

      virtual std::shared_ptr<const Symbol_Constant> operator()(const std::shared_ptr<const Node_Action::Data> action_data) const = 0;
    };

    class Get_Symbol_Constant : public Get_Symbol {
    public:
      Get_Symbol_Constant(const std::shared_ptr<const Symbol_Constant> symbol) : m_symbol(symbol) {}

      std::shared_ptr<const Symbol_Constant> operator()(const std::shared_ptr<const Node_Action::Data>) const override {
        return m_symbol;
      }

    private:
      const std::shared_ptr<const Symbol_Constant> m_symbol;
    };

    class Get_Symbol_Variable : public Get_Symbol {
    public:
      Get_Symbol_Variable(const std::shared_ptr<const Symbol_Variable> variable) : m_variable(variable) {}

      std::shared_ptr<const Symbol_Constant> operator()(const std::shared_ptr<const Node_Action::Data> action_data) const override {
        assert(action_data);
        return std::dynamic_pointer_cast<const Symbol_Constant>((*action_data->token)[action_data->variable_indices->find_index(m_variable->get_value())]);
      }

    private:
      const std::shared_ptr<const Symbol_Variable> m_variable;
    };

    std::vector<std::shared_ptr<const Get_Symbol>> symbols;
    symbols.reserve(m_symbols.size());
    for (const auto gen_symbol : m_symbols) {
      const auto symbol = gen_symbol->generate(action_data);
      if (const auto variable = std::dynamic_pointer_cast<const Symbol_Variable>(symbol))
        symbols.push_back(std::make_shared<Get_Symbol_Variable>(variable));
      else
        symbols.push_back(std::make_shared<Get_Symbol_Constant>(std::dynamic_pointer_cast<const Symbol_Constant>(symbol)));
    }

    class Action_Make : public Node_Action::Action {
      Action_Make(const Action_Make &) = delete;
      Action_Make & operator=(const Action_Make &) = delete;

    public:
      Action_Make(const std::vector<std::shared_ptr<const Get_Symbol>> &symbols)
        : m_symbols(symbols)
      {
      }

      void operator()(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Node_Action::Data> action_data) const override {
        auto st = m_symbols.cbegin();
        const auto send = m_symbols.cend();
        const auto first = (**st)(action_data);
        ++st;
        while (st != send) {
          const auto second = (**st)(action_data);
          ++st;
          assert(st != send);
          const auto third = (**st)(action_data);
          ++st;

          network->insert_wme(job_queue, std::make_shared<WME>(first, second, third));
        }
      }

    private:
      const std::vector<std::shared_ptr<const Get_Symbol>> m_symbols;
    };

    return std::make_shared<Action_Make>(symbols);
  }

  Action_Production_Generator::Action_Production_Generator(const std::shared_ptr<const Node_Action_Generator> action)
    : m_action(action)
  {
  }

  std::shared_ptr<const Action_Generator> Action_Production_Generator::clone(const std::shared_ptr<const Node_Action::Data> action_data) const {
    const auto action = m_action->clone(action_data);
    if (action == m_action)
      return shared_from_this();
    else
      return std::make_shared<Action_Production_Generator>(std::dynamic_pointer_cast<const Node_Action_Generator>(action));
  }

  std::shared_ptr<const Node_Action::Action> Action_Production_Generator::generate(const std::shared_ptr<Network>, const std::shared_ptr<Concurrency::Job_Queue>, const bool user_action, const std::shared_ptr<const Node_Action::Data> action_data) const {
    class Action_Production : public Node_Action::Action {
      Action_Production(const Action_Production &) = delete;
      Action_Production & operator=(const Action_Production &) = delete;

    public:
      Action_Production(const bool user_action, const std::shared_ptr<const Node_Action_Generator> action)
        : m_user_action(user_action), m_action(action)
      {
      }

      void operator()(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node_Action>, const std::shared_ptr<const Node_Action::Data> action_data) const override {
        m_action->generate(network, job_queue, m_user_action, action_data);
      }

    private:
      const bool m_user_action;
      const std::shared_ptr<const Node_Action_Generator> m_action;
    };

    return std::make_shared<Action_Production>(user_action, std::dynamic_pointer_cast<const Node_Action_Generator>(m_action->clone(action_data)));
  }

  std::shared_ptr<const Action_Generator> Action_Remove_Generator::clone(const std::shared_ptr<const Node_Action::Data> action_data) const {
    std::vector<std::shared_ptr<const Symbol_Generator>> symbols;
    symbols.reserve(m_symbols.size());
    for (const auto symbol : m_symbols)
      symbols.push_back(symbol->clone(action_data));
    for (auto st = symbols.cbegin(), mt = m_symbols.cbegin(), send = symbols.cend(); st != send; ++st, ++mt) {
      if (*st != *mt)
        return std::make_shared<Action_Remove_Generator>(symbols);
    }
    return shared_from_this();
  }

  std::shared_ptr<const Node_Action::Action> Action_Remove_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool, const std::shared_ptr<const Node_Action::Data> action_data) const {
    class Get_Symbol : public std::enable_shared_from_this<Get_Symbol> {
    public:
      virtual ~Get_Symbol() {}

      virtual std::shared_ptr<const Symbol_Constant> operator()(const std::shared_ptr<const Node_Action::Data> action_data) const = 0;
    };

    class Get_Symbol_Constant : public Get_Symbol {
    public:
      Get_Symbol_Constant(const std::shared_ptr<const Symbol_Constant> symbol) : m_symbol(symbol) {}

      std::shared_ptr<const Symbol_Constant> operator()(const std::shared_ptr<const Node_Action::Data>) const override {
        return m_symbol;
      }

    private:
      const std::shared_ptr<const Symbol_Constant> m_symbol;
    };

    class Get_Symbol_Variable : public Get_Symbol {
    public:
      Get_Symbol_Variable(const std::shared_ptr<const Symbol_Variable> variable) : m_variable(variable) {}

      std::shared_ptr<const Symbol_Constant> operator()(const std::shared_ptr<const Node_Action::Data> action_data) const override {
        assert(action_data);
        return std::dynamic_pointer_cast<const Symbol_Constant>((*action_data->token)[action_data->variable_indices->find_index(m_variable->get_value())]);
      }

    private:
      const std::shared_ptr<const Symbol_Variable> m_variable;
    };

    std::vector<std::shared_ptr<const Get_Symbol>> symbols;
    symbols.reserve(m_symbols.size());
    for (const auto gen_symbol : m_symbols) {
      const auto symbol = gen_symbol->generate(action_data);
      if (const auto variable = std::dynamic_pointer_cast<const Symbol_Variable>(symbol))
        symbols.push_back(std::make_shared<Get_Symbol_Variable>(variable));
      else
        symbols.push_back(std::make_shared<Get_Symbol_Constant>(std::dynamic_pointer_cast<const Symbol_Constant>(symbol)));
    }

    class Action_Remove : public Node_Action::Action {
      Action_Remove(const Action_Remove &) = delete;
      Action_Remove & operator=(const Action_Remove &) = delete;

    public:
      Action_Remove(const std::vector<std::shared_ptr<const Get_Symbol>> &symbols)
        : m_symbols(symbols)
      {
      }

      void operator()(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Node_Action::Data> action_data) const override {
        auto st = m_symbols.cbegin();
        const auto send = m_symbols.cend();
        const auto first = (**st)(action_data);
        ++st;
        while (st != send) {
          const auto second = (**st)(action_data);
          ++st;
          assert(st != send);
          const auto third = (**st)(action_data);
          ++st;

          network->remove_wme(job_queue, std::make_shared<WME>(first, second, third));
        }
      }

    private:
      const std::vector<std::shared_ptr<const Get_Symbol>> m_symbols;
    };

    return std::make_shared<Action_Remove>(symbols);
  }

  Action_Write_Generator::Action_Write_Generator(const std::vector<std::shared_ptr<const Symbol_Generator>> &symbols) {
    auto st = symbols.cbegin();
    const auto send = symbols.cend();
    while (st != send) {
      if (std::dynamic_pointer_cast<const Symbol_Variable_Generator>(*st)) {
        m_symbols.push_back(*st);
        ++st;
        continue;
      }

      auto s2 = std::next(st);
      if (s2 == send) {
        m_symbols.push_back(*st);
        break;
      }

      if (std::dynamic_pointer_cast<const Symbol_Variable_Generator>(*s2)) {
        m_symbols.push_back(*st);
        m_symbols.push_back(*s2);
        st = std::next(s2);
        continue;
      }

      std::ostringstream oss;
      (*st)->generate(nullptr)->print_contents(oss);
      (*s2)->generate(nullptr)->print_contents(oss);
      st = std::next(s2);
      while (st != send && !std::dynamic_pointer_cast<const Symbol_Variable_Generator>(*st))
        (*st++)->generate(nullptr)->print_contents(oss);
      m_symbols.push_back(std::make_shared<Symbol_Constant_Generator>(std::make_shared<Symbol_Constant_String>(oss.str())));
    }
  }

  std::shared_ptr<const Action_Generator> Action_Write_Generator::clone(const std::shared_ptr<const Node_Action::Data> action_data) const {
    std::vector<std::shared_ptr<const Symbol_Generator>> symbols;
    symbols.reserve(m_symbols.size());
    for (const auto symbol : m_symbols)
      symbols.push_back(symbol->clone(action_data));
    for (auto st = symbols.cbegin(), mt = m_symbols.cbegin(), send = symbols.cend(); st != send; ++st, ++mt) {
      if (*st != *mt)
        return std::make_shared<Action_Write_Generator>(symbols);
    }
    return shared_from_this();
  }

  std::shared_ptr<const Node_Action::Action> Action_Write_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool, const std::shared_ptr<const Node_Action::Data> action_data) const {
    class Write : public std::enable_shared_from_this<Write> {
    public:
      virtual ~Write() {}

      virtual void operator()(std::ostream &os, const std::shared_ptr<const Node_Action::Data> action_data) const = 0;
    };

    class Write_String : public Write {
    public:
      Write_String(const std::string &string) : m_string(string) {}

      void operator()(std::ostream &os, const std::shared_ptr<const Node_Action::Data>) const override {
        os << m_string;
      }

    private:
      const std::string m_string;
    };

    class Write_Variable : public Write {
    public:
      Write_Variable(const std::string &variable_name) : m_variable_name(variable_name) {}

      void operator()(std::ostream &os, const std::shared_ptr<const Node_Action::Data> action_data) const override {
        assert(action_data);
        (*action_data->token)[action_data->variable_indices->find_index(m_variable_name)]->print_contents(os);
      }

    private:
      const std::string m_variable_name;
    };

    std::vector<std::shared_ptr<const Write>> writes;
    std::ostringstream oss;
    for (const auto gen_symbol : m_symbols) {
      const auto symbol = gen_symbol->generate(action_data);
      if (const auto variable = std::dynamic_pointer_cast<const Symbol_Variable>(symbol)) {
        if (!oss.str().empty()) {
          writes.push_back(std::make_shared<Write_String>(oss.str()));
          oss.str("");
        }

        assert(strlen(variable->get_value()) != 0);

        writes.push_back(std::make_shared<Write_Variable>(variable->get_value()));
      }
      else
        symbol->print_contents(oss);
    }
    if (!oss.str().empty()) {
      writes.push_back(std::make_shared<Write_String>(oss.str()));
      oss.str("");
    }

    class Action_Write : public Node_Action::Action {
      Action_Write(const Action_Write &) = delete;
      Action_Write & operator=(const Action_Write &) = delete;

    public:
      Action_Write(const std::vector<std::shared_ptr<const Write>> &writes)
        : m_writes(writes)
      {
      }

      void operator()(const std::shared_ptr<Network>, const std::shared_ptr<Concurrency::Job_Queue>, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Node_Action::Data> action_data) const override {
        std::ostringstream oss;
        for (const auto write : m_writes)
          (*write)(oss, action_data);
        std::cout << oss.str();
      }

    private:
      const std::vector<std::shared_ptr<const Write>> m_writes;
    };

    return std::make_shared<Action_Write>(writes);
  }

  Node_Action_Generator::Node_Action_Generator(const std::shared_ptr<const Symbol_Generator> name_, const std::shared_ptr<const Node_Generator> input_, const std::shared_ptr<const Action_Generator> action_, const std::shared_ptr<const Action_Generator> retraction_)
    : name(name_), input(input_), action(action_), retraction(retraction_)
  {
  }

  std::shared_ptr<const Node_Generator> Node_Action_Generator::clone(const std::shared_ptr<const Node_Action::Data> action_data) const {
    const auto name0 = name->clone(action_data);
    const auto input0 = input->clone(action_data);
    const auto action0 = action->clone(action_data);
    const auto retraction0 = retraction->clone(action_data);
    if (name0 == name && input0 == input && action0 == action && retraction0 == retraction)
      return shared_from_this();
    else
      return std::make_shared<Node_Action_Generator>(name0, input0, action0, retraction0);
  }

  std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>,
    std::vector<std::tuple<Token_Index, std::shared_ptr<const Zeni::Rete::Node_Predicate::Predicate>, std::shared_ptr<const Symbol_Variable>>>>
    Node_Action_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const std::shared_ptr<const Node_Action::Data> action_data) const
  {
    const auto name_gen0 = name->generate(action_data);
    const auto [input0, key0, variable_indices0, predicates0] = input->generate(network, job_queue, user_action, action_data);

    assert(predicates0.empty());

    std::ostringstream oss;
    name_gen0->print_contents(oss);

    std::string name0 = oss.str();
    name0.erase(std::remove_if(name0.begin(), name0.end(), [](const char c) { return c == '\r' || c == '\n' || c == '|'; }), name0.end());

    const auto action0 = action->generate(network, job_queue, user_action, action_data);
    const auto retraction0 = retraction->generate(network, job_queue, user_action, action_data);

    const auto node = Node_Action::Create(network, job_queue, name0, user_action, key0, input0, variable_indices0, action0, retraction0);

    return std::make_tuple(node, Node_Key_Null::Create(), dynamic_cast<const Node_Action *>(node.get())->get_variable_indices(), predicates0);
  }

  Node_Filter_Generator::Node_Filter_Generator(const std::shared_ptr<const Symbols> first_, const std::shared_ptr<const Symbols> second_, const std::shared_ptr<const Symbols> third_)
    : first(first_), second(second_), third(third_)
  {
  }

  std::shared_ptr<const Node_Generator> Node_Filter_Generator::clone(const std::shared_ptr<const Node_Action::Data> action_data) const {
    bool different = false;

    std::shared_ptr<Symbols> symbols0 = std::make_shared<Symbols>(*first);
    for (auto &gen : symbols0->symbols) {
      const auto newgen = gen->clone(action_data);
      if (gen != newgen) {
        different = true;
        gen = newgen;
      }
    }
    for (auto &pred : symbols0->predicates) {
      const auto newgen = pred.second->clone(action_data);
      if (pred.second != newgen) {
        different = true;
        pred.second = newgen;
      }
    }

    std::shared_ptr<Symbols> symbols1 = std::make_shared<Symbols>(*second);
    for (auto &gen : symbols1->symbols) {
      const auto newgen = gen->clone(action_data);
      if (gen != newgen) {
        different = true;
        gen = newgen;
      }
    }
    for (auto &pred : symbols1->predicates) {
      const auto newgen = pred.second->clone(action_data);
      if (pred.second != newgen) {
        different = true;
        pred.second = newgen;
      }
    }

    std::shared_ptr<Symbols> symbols2 = std::make_shared<Symbols>(*third);
    for (auto &gen : symbols2->symbols) {
      const auto newgen = gen->clone(action_data);
      if (gen != newgen) {
        different = true;
        gen = newgen;
      }
    }
    for (auto &pred : symbols2->predicates) {
      const auto newgen = pred.second->clone(action_data);
      if (pred.second != newgen) {
        different = true;
        pred.second = newgen;
      }
    }
    
    return different ? std::make_shared<Node_Filter_Generator>(symbols0, symbols1, symbols2) : shared_from_this();
  }

  std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>,
    std::vector<std::tuple<Token_Index, std::shared_ptr<const Zeni::Rete::Node_Predicate::Predicate>, std::shared_ptr<const Symbol_Variable>>>>
    Node_Filter_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool, const std::shared_ptr<const Node_Action::Data> action_data) const
  {
    std::shared_ptr<Zeni::Rete::Node> node = network;
    std::shared_ptr<const Node_Key> key;
    auto variable_indices = Variable_Indices::Create();
    std::vector<std::tuple<Token_Index, std::shared_ptr<const Zeni::Rete::Node_Predicate::Predicate>, std::shared_ptr<const Symbol_Variable>>> predicates;

    if (first->symbols.size() > 1) {
      Node_Key_Multisym::Node_Key_Symbol_Trie symbol_trie;
      for (const auto gen : first->symbols)
        symbol_trie.insert(Node_Key_Symbol::Create(gen->generate(action_data)));
      key = Node_Key_Multisym::Create(symbol_trie);
    }
    else if (!first->symbols.empty())
      key = Node_Key_Symbol::Create(first->symbols.front()->generate(action_data));
    else
      key = Node_Key_Null::Create();

    if (first->variable)
      variable_indices->insert(first->variable->get_value(), Token_Index(0, 0, 0));

    for (const auto pred : first->predicates) {
      const auto sym_right = pred.second->generate(action_data);
      std::shared_ptr<const Node_Predicate::Get_Symbol> rhs;
      if (const auto var = std::dynamic_pointer_cast<const Symbol_Variable>(sym_right)) {
        const auto index_right = variable_indices->find_index(var->get_value());
        if (index_right == Token_Index()) {
          predicates.push_back(std::make_tuple(Token_Index(0, 0, 0), pred.first, var));
          continue;
        }
        else
          rhs = std::make_shared<Node_Predicate::Get_Symbol_Variable>(index_right);
      }
      else
        rhs = std::make_shared<Node_Predicate::Get_Symbol_Constant>(sym_right);
      node = Node_Predicate::Create(network, job_queue, key, node, pred.first, Token_Index(0, 0, 0), rhs);
      key = Node_Key_Null::Create();
    }


    if (second->symbols.size() > 1) {
      node = Node_Filter_1::Create(network, job_queue, key);
      Node_Key_Multisym::Node_Key_Symbol_Trie symbol_trie;
      for (const auto gen : second->symbols)
        symbol_trie.insert(Node_Key_Symbol::Create(gen->generate(action_data)));
      key = Node_Key_Multisym::Create(symbol_trie);
    }
    else if (!second->symbols.empty()) {
      node = Node_Filter_1::Create(network, job_queue, key);
      key = Node_Key_Symbol::Create(second->symbols.front()->generate(action_data));
    }
    else if (second->variable && first->variable && *second->variable == *first->variable) {
      node = Node_Filter_1::Create(network, job_queue, key);
      key = Node_Key_01::Create();
    }

    if (second->variable)
      variable_indices->insert(second->variable->get_value(), Token_Index(0, 0, 1));

    for (const auto pred : second->predicates) {
      const auto sym_right = pred.second->generate(action_data);
      std::shared_ptr<const Node_Predicate::Get_Symbol> rhs;
      if (const auto var = std::dynamic_pointer_cast<const Symbol_Variable>(sym_right)) {
        const auto index_right = variable_indices->find_index(var->get_value());
        if (index_right == Token_Index()) {
          predicates.push_back(std::make_tuple(Token_Index(0, 0, 1), pred.first, var));
          continue;
        }
        else
          rhs = std::make_shared<Node_Predicate::Get_Symbol_Variable>(index_right);
      }
      else
        rhs = std::make_shared<Node_Predicate::Get_Symbol_Constant>(sym_right);
      node = Node_Predicate::Create(network, job_queue, key, node, pred.first, Token_Index(0, 0, 1), rhs);
      key = Node_Key_Null::Create();
    }


    if (third->symbols.size() > 1) {
      node = Node_Filter_2::Create(network, job_queue, key, node);
      Node_Key_Multisym::Node_Key_Symbol_Trie symbol_trie;
      for (const auto gen : third->symbols)
        symbol_trie.insert(Node_Key_Symbol::Create(gen->generate(action_data)));
      key = Node_Key_Multisym::Create(symbol_trie);
    }
    else if (!third->symbols.empty()) {
      node = Node_Filter_2::Create(network, job_queue, key, node);
      key = Node_Key_Symbol::Create(third->symbols.front()->generate(action_data));
    }
    else if (third->variable && first->variable && *third->variable == *first->variable) {
      node = Node_Filter_2::Create(network, job_queue, key, node);
      key = Node_Key_02::Create();
    }
    else if (third->variable && second->variable && *third->variable == *second->variable) {
      node = Node_Filter_2::Create(network, job_queue, key, node);
      key = Node_Key_12::Create();
    }

    if (third->variable)
      variable_indices->insert(third->variable->get_value(), Token_Index(0, 0, 2));

    for (const auto pred : third->predicates) {
      const auto sym_right = pred.second->generate(action_data);
      std::shared_ptr<const Node_Predicate::Get_Symbol> rhs;
      if (const auto var = std::dynamic_pointer_cast<const Symbol_Variable>(sym_right)) {
        const auto index_right = variable_indices->find_index(var->get_value());
        if (index_right == Token_Index()) {
          predicates.push_back(std::make_tuple(Token_Index(0, 0, 2), pred.first, var));
          continue;
        }
        else
          rhs = std::make_shared<Node_Predicate::Get_Symbol_Variable>(index_right);
      }
      else
        rhs = std::make_shared<Node_Predicate::Get_Symbol_Constant>(sym_right);
      node = Node_Predicate::Create(network, job_queue, key, node, pred.first, Token_Index(0, 0, 2), rhs);
      key = Node_Key_Null::Create();
    }

    return std::make_tuple(node, key, variable_indices, predicates);
  }

  Node_Join_Generator::Node_Join_Generator(const std::shared_ptr<const Node_Generator> left_, const std::shared_ptr<const Node_Generator> right_)
    : left(left_), right(right_)
  {
  }

  std::shared_ptr<const Node_Generator> Node_Join_Generator::clone(const std::shared_ptr<const Node_Action::Data> action_data) const {
    const auto node0 = left->clone(action_data);
    const auto node1 = right->clone(action_data);
    if (node0 == left && node1 == right)
      return shared_from_this();
    else
      return std::make_shared<Node_Join_Generator>(node0, node1);
  }

  std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>,
    std::vector<std::tuple<Token_Index, std::shared_ptr<const Zeni::Rete::Node_Predicate::Predicate>, std::shared_ptr<const Symbol_Variable>>>>
    Node_Join_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool, const std::shared_ptr<const Node_Action::Data> action_data) const
  {
    const auto[node0, key0, indices0, predicates0] = left->generate(network, job_queue, false, action_data);
    const auto[node1, key1, indices1, predicates1] = right->generate(network, job_queue, false, action_data);

    Variable_Bindings variable_bindings;
    for (auto index_right : indices1->get_indices()) {
      auto index_left = indices0->find_index(index_right.first);
      if (index_left != Token_Index())
        variable_bindings.emplace(index_left, index_right.second);
    }

    std::shared_ptr<Node> node = Node_Join::Create(network, job_queue, key0, key1, node0, node1, std::move(variable_bindings));
    const auto variable_indices = Variable_Indices::Create(node0->get_size(), node0->get_token_size(), *indices0, *indices1);

    std::remove_const_t<decltype(predicates0)> predicates;
    for (const auto pred : predicates0) {
      const auto index_right = variable_indices->find_index(std::get<2>(pred)->get_value());
      if (index_right == Token_Index())
        predicates.emplace_back(pred);
      else {
        const auto rhs = std::make_shared<Node_Predicate::Get_Symbol_Variable>(index_right);
        node = Node_Predicate::Create(network, job_queue, Node_Key_Null::Create(), node, std::get<1>(pred), std::get<0>(pred), rhs);
      }
    }
    for (const auto pred : predicates1) {
      const auto index_right = variable_indices->find_index(std::get<2>(pred)->get_value());
      if (index_right == Token_Index())
        predicates.emplace_back(pred);
      else {
        auto lhs = std::get<0>(pred);
        lhs.rete_row += node0->get_size();
        lhs.token_row += node0->get_token_size();
        const auto rhs = std::make_shared<Node_Predicate::Get_Symbol_Variable>(index_right);
        node = Node_Predicate::Create(network, job_queue, Node_Key_Null::Create(), node, std::get<1>(pred), lhs, rhs);
      }
    }

    return std::make_tuple(node, Node_Key_Null::Create(), variable_indices, predicates);
  }

  Node_Join_Existential_Generator::Node_Join_Existential_Generator(const std::shared_ptr<const Node_Generator> left_, const std::shared_ptr<const Node_Generator> right_)
    : left(left_), right(right_)
  {
  }

  std::shared_ptr<const Node_Generator> Node_Join_Existential_Generator::clone(const std::shared_ptr<const Node_Action::Data> action_data) const {
    const auto node0 = left->clone(action_data);
    const auto node1 = right->clone(action_data);
    if (node0 == left && node1 == right)
      return shared_from_this();
    else
      return std::make_shared<Node_Join_Existential_Generator>(node0, node1);
  }

  std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>,
    std::vector<std::tuple<Token_Index, std::shared_ptr<const Zeni::Rete::Node_Predicate::Predicate>, std::shared_ptr<const Symbol_Variable>>>>
    Node_Join_Existential_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool, const std::shared_ptr<const Node_Action::Data> action_data) const
  {
    const auto[node0, key0, indices0, predicates0] = left->generate(network, job_queue, false, action_data);
    const auto[node1, key1, indices1, predicates1] = right->generate(network, job_queue, false, action_data);

    Variable_Bindings variable_bindings;
    for (auto index_right : indices1->get_indices()) {
      auto index_left = indices0->find_index(index_right.first);
      if (index_left != Token_Index())
        variable_bindings.emplace(index_left, index_right.second);
    }

    std::shared_ptr<Node> node = Node_Join_Existential::Create(network, job_queue, key0, key1, node0, node1, std::move(variable_bindings));
    const auto variable_indices = Variable_Indices::Create(node0->get_size(), node0->get_token_size(), *indices0, *indices1);

    std::remove_const_t<decltype(predicates0)> predicates;
    for (const auto pred : predicates0) {
      const auto index_right = variable_indices->find_index(std::get<2>(pred)->get_value());
      if (index_right == Token_Index())
        predicates.emplace_back(pred);
      else {
        const auto rhs = std::make_shared<Node_Predicate::Get_Symbol_Variable>(index_right);
        node = Node_Predicate::Create(network, job_queue, Node_Key_Null::Create(), node, std::get<1>(pred), std::get<0>(pred), rhs);
      }
    }
    for (const auto pred : predicates1) {
      const auto index_right = variable_indices->find_index(std::get<2>(pred)->get_value());
      if (index_right == Token_Index())
        predicates.emplace_back(pred);
      else {
        auto lhs = std::get<0>(pred);
        lhs.rete_row += node0->get_size();
        lhs.token_row += node0->get_token_size();
        const auto rhs = std::make_shared<Node_Predicate::Get_Symbol_Variable>(index_right);
        node = Node_Predicate::Create(network, job_queue, Node_Key_Null::Create(), node, std::get<1>(pred), lhs, rhs);
      }
    }

    return std::make_tuple(node, Node_Key_Null::Create(), variable_indices, predicates);
  }

  Node_Join_Negation_Generator::Node_Join_Negation_Generator(const std::shared_ptr<const Node_Generator> left_, const std::shared_ptr<const Node_Generator> right_)
    : left(left_), right(right_)
  {
  }

  std::shared_ptr<const Node_Generator> Node_Join_Negation_Generator::clone(const std::shared_ptr<const Node_Action::Data> action_data) const {
    const auto node0 = left->clone(action_data);
    const auto node1 = right->clone(action_data);
    if (node0 == left && node1 == right)
      return shared_from_this();
    else
      return std::make_shared<Node_Join_Negation_Generator>(node0, node1);
  }

  std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>,
    std::vector<std::tuple<Token_Index, std::shared_ptr<const Zeni::Rete::Node_Predicate::Predicate>, std::shared_ptr<const Symbol_Variable>>>>
    Node_Join_Negation_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool, const std::shared_ptr<const Node_Action::Data> action_data) const
  {
    const auto[node0, key0, indices0, predicates0] = left->generate(network, job_queue, false, action_data);
    const auto[node1, key1, indices1, predicates1] = right->generate(network, job_queue, false, action_data);

    Variable_Bindings variable_bindings;
    for (auto index_right : indices1->get_indices()) {
      auto index_left = indices0->find_index(index_right.first);
      if (index_left != Token_Index())
        variable_bindings.emplace(index_left, index_right.second);
    }

    std::shared_ptr<Node> node = Node_Join_Negation::Create(network, job_queue, key0, key1, node0, node1, std::move(variable_bindings));
    const auto variable_indices = Variable_Indices::Create(node0->get_size(), node0->get_token_size(), *indices0, *indices1);

    std::remove_const_t<decltype(predicates0)> predicates;
    for (const auto pred : predicates0) {
      const auto index_right = variable_indices->find_index(std::get<2>(pred)->get_value());
      if (index_right == Token_Index())
        predicates.emplace_back(pred);
      else {
        const auto rhs = std::make_shared<Node_Predicate::Get_Symbol_Variable>(index_right);
        node = Node_Predicate::Create(network, job_queue, Node_Key_Null::Create(), node, std::get<1>(pred), std::get<0>(pred), rhs);
      }
    }
    for (const auto pred : predicates1) {
      const auto index_right = variable_indices->find_index(std::get<2>(pred)->get_value());
      if (index_right == Token_Index())
        predicates.emplace_back(pred);
      else {
        auto lhs = std::get<0>(pred);
        lhs.rete_row += node0->get_size();
        lhs.token_row += node0->get_token_size();
        const auto rhs = std::make_shared<Node_Predicate::Get_Symbol_Variable>(index_right);
        node = Node_Predicate::Create(network, job_queue, Node_Key_Null::Create(), node, std::get<1>(pred), lhs, rhs);
      }
    }

    return std::make_tuple(node, Node_Key_Null::Create(), variable_indices, predicates);
  }

  Data::Production::Production(const std::shared_ptr<Network> network_, const std::shared_ptr<Concurrency::Job_Queue> job_queue_, const bool user_action_, const std::unordered_set<std::string> &lhs_variables_)
    : network(network_), job_queue(job_queue_), user_action(user_action_), lhs_variables(lhs_variables_)
  {
    lhs.push(decltype(lhs)::value_type());
  }

  Data::Data(const std::shared_ptr<Network> network_, const std::shared_ptr<Concurrency::Job_Queue> job_queue_, const bool user_action_)
    : network(network_), job_queue(job_queue_)
  {
    productions.push(std::make_shared<Production>(network_, job_queue_, user_action_, decltype(Production::lhs_variables)()));
    productions.top()->phase = Production::Phase::PHASE_ACTIONS;
    symbols.emplace(std::make_shared<Symbols>());
  }

}
