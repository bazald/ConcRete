#include "Zeni/Rete/Parser/Data.hpp"

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

  std::shared_ptr<const Symbol_Generator> Symbol_Constant_Generator::clone(const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const {
    return shared_from_this();
  }

  std::shared_ptr<const Symbol> Symbol_Constant_Generator::generate(const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const {
    return symbol;
  }

  Symbol_Variable_Generator::Symbol_Variable_Generator(const std::shared_ptr<const Symbol_Variable> symbol_)
    : symbol(symbol_)
  {
  }

  std::shared_ptr<const Symbol_Generator> Symbol_Variable_Generator::clone(const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const {
    if (!rete_action) {
      assert(!token);
      return shared_from_this();
    }
    else
      assert(token);

    const auto found = rete_action->get_variable_indices()->find_index(symbol->get_value());
    if (found != Token_Index())
      return std::make_shared<Symbol_Constant_Generator>((*token)[found]);
    else
      return shared_from_this();
  }

  std::shared_ptr<const Symbol> Symbol_Variable_Generator::generate(const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const {
    if (!rete_action) {
      assert(!token);
      return symbol;
    }
    else
      assert(token);

    const auto found = rete_action->get_variable_indices()->find_index(symbol->get_value());
    if (found != Token_Index())
      return (*token)[found];
    else
      return symbol;
  }

  Actions_Generator::Actions_Generator(const std::vector<std::shared_ptr<const Action_Generator>> &actions)
    : m_actions(actions)
  {
  }

  std::shared_ptr<const Action_Generator> Actions_Generator::clone(const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const {
    std::vector<std::shared_ptr<const Action_Generator>> actions;
    actions.reserve(m_actions.size());
    for (const auto action : m_actions)
      actions.push_back(action->clone(rete_action, token));
    for (auto at = actions.cbegin(), mt = m_actions.cbegin(), aend = actions.cend(); at != aend; ++at, ++mt) {
      if (*at != *mt)
        return std::make_shared<Actions_Generator>(actions);
    }
    return shared_from_this();
  }

  std::shared_ptr<const Node_Action::Action> Actions_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const {
    std::vector<std::shared_ptr<const Node_Action::Action>> actions;
    actions.reserve(m_actions.size());
    for (const auto action : m_actions)
      actions.push_back(action->generate(network, job_queue, false, rete_action, token));
    
    class Actions : public Node_Action::Action {
      Actions(const Actions &) = delete;
      Actions & operator=(const Actions &) = delete;

    public:
      Actions(const std::vector<std::shared_ptr<const Node_Action::Action>> actions)
        : m_actions(actions)
      {
      }

      void operator()(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node_Action> node_action, const std::shared_ptr<const Token> token) const override {
        for (const auto action : m_actions)
          (*action)(network, job_queue, node_action, token);
      }

    private:
      const std::vector<std::shared_ptr<const Node_Action::Action>> m_actions;
    };

    return std::make_shared<Actions>(actions);
  }

  Action_Excise_Generator::Action_Excise_Generator(const std::vector<std::shared_ptr<const Symbol_Generator>> &symbols)
    : m_symbols(symbols)
  {
  }

  std::shared_ptr<const Action_Generator> Action_Excise_Generator::clone(const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const {
    std::vector<std::shared_ptr<const Symbol_Generator>> symbols;
    symbols.reserve(m_symbols.size());
    for (const auto symbol : m_symbols)
      symbols.push_back(symbol->clone(rete_action, token));
    for (auto st = symbols.cbegin(), mt = m_symbols.cbegin(), send = symbols.cend(); st != send; ++st, ++mt) {
      if (*st != *mt)
        return std::make_shared<Action_Excise_Generator>(symbols);
    }
    return shared_from_this();
  }

  std::shared_ptr<const Node_Action::Action> Action_Excise_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const {
    class Get_Symbol : public std::enable_shared_from_this<Get_Symbol> {
    public:
      virtual ~Get_Symbol() {}

      virtual std::shared_ptr<const Symbol_Constant> operator()(const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const = 0;
    };

    class Get_Symbol_Constant : public Get_Symbol {
    public:
      Get_Symbol_Constant(const std::shared_ptr<const Symbol_Constant> symbol_) : symbol(symbol_) {}

      std::shared_ptr<const Symbol_Constant> operator()(const std::shared_ptr<Node_Action>, const std::shared_ptr<const Token>) const override {
        return symbol;
      }

      const std::shared_ptr<const Symbol_Constant> symbol;
    };

    class Get_Symbol_Variable : public Get_Symbol {
    public:
      Get_Symbol_Variable(const std::shared_ptr<const Symbol_Variable> variable_) : variable(variable_) {}

      std::shared_ptr<const Symbol_Constant> operator()(const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const override {
        assert(rete_action);
        assert(token);
        return std::dynamic_pointer_cast<const Symbol_Constant>((*token)[rete_action->get_variable_indices()->find_index(variable->get_value())]);
      }

      const std::shared_ptr<const Symbol_Variable> variable;
    };

    std::vector<std::shared_ptr<const Get_Symbol>> symbols;
    symbols.reserve(m_symbols.size());
    for (const auto gen_symbol : m_symbols) {
      const auto symbol = gen_symbol->generate(rete_action, token);
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

      void operator()(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const override {
        if (m_symbols.size() == 1) {
          if (const auto constant = std::dynamic_pointer_cast<const Get_Symbol_Constant>(m_symbols.back())) {
            if (const auto str = std::dynamic_pointer_cast<const Symbol_Constant_String>(constant->symbol)) {
              if (*str == "*") {
                network->excise_all(job_queue, m_user_action);
                return;
              }
            }
          }
        }

        for (const auto symbol : m_symbols) {
          std::ostringstream oss;
          oss << *(*symbol)(rete_action, token);
          network->excise_rule(job_queue, oss.str(), m_user_action);
        }
      }

    private:
      const std::vector<std::shared_ptr<const Get_Symbol>> m_symbols;
      const bool m_user_action;
    };

    return std::make_shared<Action_Excise>(symbols, user_action);
  }

  std::shared_ptr<const Action_Exit_Generator> Action_Exit_Generator::Create() {
    class Friendly_Action_Exit_Generator : public Action_Exit_Generator {
    public:
      Friendly_Action_Exit_Generator() {}
    };

    static const auto action_exit_generator = std::make_shared<Friendly_Action_Exit_Generator>();

    return action_exit_generator;
  }

  std::shared_ptr<const Action_Generator> Action_Exit_Generator::clone(const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const {
    return shared_from_this();
  }

  std::shared_ptr<const Node_Action::Action> Action_Exit_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const {
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

      void operator()(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue>, const std::shared_ptr<Node_Action>, const std::shared_ptr<const Token>) const override {
        network->request_exit();
      }
    };

    return Action_Exit::Create();
  }

  Action_Make_Generator::Action_Make_Generator(const std::vector<std::shared_ptr<const Symbol_Generator>> &symbols)
    : m_symbols(symbols)
  {
  }

  std::shared_ptr<const Action_Generator> Action_Make_Generator::clone(const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const {
    std::vector<std::shared_ptr<const Symbol_Generator>> symbols;
    symbols.reserve(m_symbols.size());
    for (const auto symbol : m_symbols)
      symbols.push_back(symbol->clone(rete_action, token));
    for (auto st = symbols.cbegin(), mt = m_symbols.cbegin(), send = symbols.cend(); st != send; ++st, ++mt) {
      if (*st != *mt)
        return std::make_shared<Action_Make_Generator>(symbols);
    }
    return shared_from_this();
  }

  std::shared_ptr<const Node_Action::Action> Action_Make_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const {
    class Get_Symbol : public std::enable_shared_from_this<Get_Symbol> {
    public:
      virtual ~Get_Symbol() {}

      virtual std::shared_ptr<const Symbol_Constant> operator()(const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const = 0;
    };

    class Get_Symbol_Constant : public Get_Symbol {
    public:
      Get_Symbol_Constant(const std::shared_ptr<const Symbol_Constant> symbol) : m_symbol(symbol) {}

      std::shared_ptr<const Symbol_Constant> operator()(const std::shared_ptr<Node_Action>, const std::shared_ptr<const Token>) const override {
        return m_symbol;
      }

    private:
      const std::shared_ptr<const Symbol_Constant> m_symbol;
    };

    class Get_Symbol_Variable : public Get_Symbol {
    public:
      Get_Symbol_Variable(const std::shared_ptr<const Symbol_Variable> variable) : m_variable(variable) {}

      std::shared_ptr<const Symbol_Constant> operator()(const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const override {
        assert(rete_action);
        assert(token);
        return std::dynamic_pointer_cast<const Symbol_Constant>((*token)[rete_action->get_variable_indices()->find_index(m_variable->get_value())]);
      }

    private:
      const std::shared_ptr<const Symbol_Variable> m_variable;
    };

    std::vector<std::shared_ptr<const Get_Symbol>> symbols;
    symbols.reserve(m_symbols.size());
    for (const auto gen_symbol : m_symbols) {
      const auto symbol = gen_symbol->generate(rete_action, token);
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

      void operator()(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const override {
        auto st = m_symbols.cbegin();
        const auto send = m_symbols.cend();
        const auto first = (**st)(rete_action, token);
        ++st;
        while (st != send) {
          const auto second = (**st)(rete_action, token);
          ++st;
          assert(st != send);
          const auto third = (**st)(rete_action, token);
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

  std::shared_ptr<const Action_Generator> Action_Production_Generator::clone(const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const {
    const auto action = m_action->clone(rete_action, token);
    if (action == m_action)
      return shared_from_this();
    else
      return std::make_shared<Action_Production_Generator>(std::dynamic_pointer_cast<const Node_Action_Generator>(action));
  }

  std::shared_ptr<const Node_Action::Action> Action_Production_Generator::generate(const std::shared_ptr<Network>, const std::shared_ptr<Concurrency::Job_Queue>, const bool user_action, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const {
    class Action_Production : public Node_Action::Action {
      Action_Production(const Action_Production &) = delete;
      Action_Production & operator=(const Action_Production &) = delete;

    public:
      Action_Production(const bool user_action, const std::shared_ptr<const Node_Action_Generator> action)
        : m_user_action(user_action), m_action(action)
      {
      }

      void operator()(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const override {
        m_action->generate(network, job_queue, m_user_action, rete_action, token);
      }

    private:
      const bool m_user_action;
      const std::shared_ptr<const Node_Action_Generator> m_action;
    };

    return std::make_shared<Action_Production>(user_action, std::dynamic_pointer_cast<const Node_Action_Generator>(m_action->clone(rete_action, token)));
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
      (*st)->generate(nullptr, nullptr)->print_contents(oss);
      (*s2)->generate(nullptr, nullptr)->print_contents(oss);
      st = std::next(s2);
      while (st != send && !std::dynamic_pointer_cast<const Symbol_Variable_Generator>(*st))
        (*st++)->generate(nullptr, nullptr)->print_contents(oss);
      m_symbols.push_back(std::make_shared<Symbol_Constant_Generator>(std::make_shared<Symbol_Constant_String>(oss.str())));
    }
  }

  std::shared_ptr<const Action_Generator> Action_Write_Generator::clone(const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const {
    std::vector<std::shared_ptr<const Symbol_Generator>> symbols;
    symbols.reserve(m_symbols.size());
    for (const auto symbol : m_symbols)
      symbols.push_back(symbol->clone(rete_action, token));
    for (auto st = symbols.cbegin(), mt = m_symbols.cbegin(), send = symbols.cend(); st != send; ++st, ++mt) {
      if (*st != *mt)
        return std::make_shared<Action_Write_Generator>(symbols);
    }
    return shared_from_this();
  }

  std::shared_ptr<const Node_Action::Action> Action_Write_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const {
    class Write : public std::enable_shared_from_this<Write> {
    public:
      virtual ~Write() {}

      virtual void operator()(std::ostream &os, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const = 0;
    };

    class Write_String : public Write {
    public:
      Write_String(const std::string &string) : m_string(string) {}

      void operator()(std::ostream &os, const std::shared_ptr<Node_Action>, const std::shared_ptr<const Token>) const override {
        os << m_string;
      }

    private:
      const std::string m_string;
    };

    class Write_Variable : public Write {
    public:
      Write_Variable(const std::string &variable_name) : m_variable_name(variable_name) {}

      void operator()(std::ostream &os, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const override {
        assert(rete_action);
        assert(token);
        (*token)[rete_action->get_variable_indices()->find_index(m_variable_name)]->print_contents(os);
      }

    private:
      const std::string m_variable_name;
    };

    std::vector<std::shared_ptr<const Write>> writes;
    std::ostringstream oss;
    for (const auto gen_symbol : m_symbols) {
      const auto symbol = gen_symbol->generate(rete_action, token);
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

      void operator()(const std::shared_ptr<Network>, const std::shared_ptr<Concurrency::Job_Queue>, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const override {
        std::ostringstream oss;
        for (const auto write : m_writes)
          (*write)(oss, rete_action, token);
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

  std::shared_ptr<const Node_Generator> Node_Action_Generator::clone(const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const {
    const auto name0 = name->clone(rete_action, token);
    const auto input0 = input->clone(rete_action, token);
    const auto action0 = action->clone(rete_action, token);
    const auto retraction0 = retraction->clone(rete_action, token);
    if (name0 == name && input0 == input && action0 == action && retraction0 == retraction)
      return shared_from_this();
    else
      return std::make_shared<Node_Action_Generator>(name0, input0, action0, retraction0);
  }

  std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>>
    Node_Action_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const
  {
    const auto name_gen0 = name->generate(rete_action, token);
    const auto [input0, key0, variable_indices0] = input->generate(network, job_queue, user_action, rete_action, token);

    std::ostringstream oss;
    name_gen0->print_contents(oss);

    std::string name0 = oss.str();
    name0.erase(std::remove_if(name0.begin(), name0.end(), [](const char c) { return c == '\r' || c == '\n' || c == '|'; }), name0.end());

    const auto action0 = action->generate(network, job_queue, user_action, rete_action, token);
    const auto retraction0 = retraction->generate(network, job_queue, user_action, rete_action, token);

    const auto node = Node_Action::Create(network, job_queue, name0, user_action, key0, input0, variable_indices0, action0, retraction0);

    return std::make_tuple(node, Node_Key_Null::Create(), dynamic_cast<const Node_Action *>(node.get())->get_variable_indices());
  }

  Node_Filter_Generator::Node_Filter_Generator(const std::shared_ptr<const Symbol_Generator> first_, const std::shared_ptr<const Symbol_Generator> second_, const std::shared_ptr<const Symbol_Generator> third_)
    : first(first_), second(second_), third(third_)
  {
  }

  std::shared_ptr<const Node_Generator> Node_Filter_Generator::clone(const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const {
    const auto symbol0 = first->clone(rete_action, token);
    const auto symbol1 = second->clone(rete_action, token);
    const auto symbol2 = third->clone(rete_action, token);
    if (symbol0 == first && symbol1 == second && symbol2 == third)
      return shared_from_this();
    else
      return std::make_shared<Node_Filter_Generator>(symbol0, symbol1, symbol2);
  }

  std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>>
    Node_Filter_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const
  {
    const auto symbol0 = first->generate(rete_action, token);
    const auto symbol1 = second->generate(rete_action, token);
    const auto symbol2 = third->generate(rete_action, token);

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

  std::shared_ptr<const Node_Generator> Node_Join_Generator::clone(const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const {
    const auto node0 = left->clone(rete_action, token);
    const auto node1 = right->clone(rete_action, token);
    if (node0 == left && node1 == right)
      return shared_from_this();
    else
      return std::make_shared<Node_Join_Generator>(node0, node1);
  }

  std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>>
    Node_Join_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const
  {
    const auto[node0, key0, indices0] = left->generate(network, job_queue, false, rete_action, token);
    const auto[node1, key1, indices1] = right->generate(network, job_queue, false, rete_action, token);

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

  std::shared_ptr<const Node_Generator> Node_Join_Existential_Generator::clone(const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const {
    const auto node0 = left->clone(rete_action, token);
    const auto node1 = right->clone(rete_action, token);
    if (node0 == left && node1 == right)
      return shared_from_this();
    else
      return std::make_shared<Node_Join_Existential_Generator>(node0, node1);
  }

  std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>>
    Node_Join_Existential_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const
  {
    const auto[node0, key0, indices0] = left->generate(network, job_queue, false, rete_action, token);
    const auto[node1, key1, indices1] = right->generate(network, job_queue, false, rete_action, token);

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

  std::shared_ptr<const Node_Generator> Node_Join_Negation_Generator::clone(const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const {
    const auto node0 = left->clone(rete_action, token);
    const auto node1 = right->clone(rete_action, token);
    if (node0 == left && node1 == right)
      return shared_from_this();
    else
      return std::make_shared<Node_Join_Negation_Generator>(node0, node1);
  }

  std::tuple<std::shared_ptr<Node>, std::shared_ptr<const Node_Key>, std::shared_ptr<const Variable_Indices>>
    Node_Join_Negation_Generator::generate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token) const
  {
    const auto[node0, key0, indices0] = left->generate(network, job_queue, false, rete_action, token);
    const auto[node1, key1, indices1] = right->generate(network, job_queue, false, rete_action, token);

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
  }

}
