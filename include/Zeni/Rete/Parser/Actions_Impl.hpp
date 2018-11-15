#ifndef ZENI_RETE_PARSER_ACTIONS_IMPL_HPP
#define ZENI_RETE_PARSER_ACTIONS_IMPL_HPP

#include "Actions.hpp"

#include "Zeni/Concurrency/Worker_Threads.hpp"
#include "Zeni/Rete/Parser/Data.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Node_Filter_1.hpp"
#include "Zeni/Rete/Node_Filter_2.hpp"
#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Join.hpp"
#include "Zeni/Rete/Node_Join_Existential.hpp"
#include "Zeni/Rete/Node_Join_Negation.hpp"
#include "Zeni/Rete/Parser.hpp"

namespace Zeni::Rete::PEG {

  /// Symbols

  template <typename Input>
  void Action<Constant_Float>::apply(const Input &input, Data &data) {
    const double d = std::stod(input.string());
    //std::cerr << "Constant_Float: " << d << std::endl;
    const auto symbol = std::make_shared<Symbol_Constant_Float>(d);
    data.symbols.emplace_back(std::make_shared<Symbol_Constant_Generator>(symbol));
  }

  template<typename Input>
  void Action<Constant_Int>::apply(const Input &input, Data &data) {
    const int64_t i = std::stoll(input.string());
    //std::cerr << "Constant_Int: " << i << std::endl;
    const auto symbol = std::make_shared<Symbol_Constant_Int>(i);
    data.symbols.emplace_back(std::make_shared<Symbol_Constant_Generator>(symbol));
  }

  template<typename Input>
  void Action<Constant_String>::apply(const Input &input, Data &data) {
    //std::cerr << "Constant_String: " << input.string() << std::endl;
    const auto symbol = std::make_shared<Symbol_Constant_String>(input.string());
    data.symbols.emplace_back(std::make_shared<Symbol_Constant_Generator>(symbol));
  }

  template<typename Input>
  void Action<Quoted_Constant_String>::apply(const Input &input, Data &data) {
    //std::cerr << "Quoted_Constant_String: " << input.string() << std::endl;
    assert(input.string().size() > 2);
    const auto substr = input.string().substr(1, input.string().size() - 2);
    const auto symbol = std::make_shared<Symbol_Constant_String>(substr);
    data.symbols.emplace_back(std::make_shared<Symbol_Constant_Generator>(symbol));
  }

  template<typename Input>
  void Action<CRLF>::apply(const Input &input, Data &data) {
    //std::cerr << "CRLF: " << input.string() << std::endl;
    const auto symbol = std::make_shared<Symbol_Constant_String>("\r\n");
    data.symbols.emplace_back(std::make_shared<Symbol_Constant_Generator>(symbol));
  }

  template<typename Input>
  void Action<Variable>::apply(const Input &input, Data &data) {
    //std::cerr << "Constant_Variable: " << input.string() << std::endl;
    assert(input.string().size() > 2);
    const auto substr = input.string().substr(1, input.string().size() - 2);
    const auto symbol = std::make_shared<Symbol_Variable>(substr.c_str());
    data.symbols.emplace_back(std::make_shared<Symbol_Variable_Generator>(symbol));
  }

  template<typename Input>
  void Action<Unnamed_Variable>::apply(const Input &input, Data &data) {
    //std::cerr << "Unnamed_Variable: " << input.string() << std::endl;
    const auto symbol = std::make_shared<Symbol_Variable>("");
    data.symbols.emplace_back(std::make_shared<Symbol_Variable_Generator>(symbol));
  }

  /// Conditions and WMEs

  template<typename Input>
  void Action<Condition_Begin>::apply(const Input &input, Data &data) {
    //std::cerr << "Condition_Begin: " << input.string() << std::endl;

    data.productions.top()->lhs.emplace(std::stack<std::pair<std::pair<std::shared_ptr<Zeni::Rete::Node>, std::shared_ptr<const Node_Key>>, std::shared_ptr<Variable_Indices>>>());
  }

  template<typename Input>
  void Action<Condition_Attr_Value>::apply(const Input &input, Data &data) {
    //std::cerr << "Condition_Attr_Value: " << input.string() << std::endl;

    assert(data.symbols.size() == 3);
    const auto gen_third = data.symbols.back();
    const auto third = gen_third->generate(data.productions.top()->substitutions);
    const auto third_var = std::dynamic_pointer_cast<const Symbol_Variable>(third);
    data.symbols.pop_back();
    const auto gen_second = data.symbols.back();
    const auto second = gen_second->generate(data.productions.top()->substitutions);
    const auto second_var = std::dynamic_pointer_cast<const Symbol_Variable>(second);
    data.symbols.pop_back();
    const auto gen_first = data.symbols.back();
    const auto first = gen_first->generate(data.productions.top()->substitutions);
    const auto first_var = std::dynamic_pointer_cast<const Symbol_Variable>(first);
    // Deliberately leave the first symbol on the stack

    std::shared_ptr<Zeni::Rete::Node> node = data.network;
    auto variable_indices = Variable_Indices::Create();

    std::shared_ptr<const Node_Key> key;
    if (first_var) {
      key = Node_Key_Null::Create();
      if (strlen(first_var->get_value()) != 0)
        variable_indices->insert(first_var->get_value(), Token_Index(0, 0, 0));
    }
    else
      key = Node_Key_Symbol::Create(first);

    if (second_var) {
      if (strlen(second_var->get_value()) != 0) {
        if (first_var && *first_var == *second_var) {
          node = Node_Filter_1::Create(data.network, data.job_queue, key);
          key = Node_Key_01::Create();
        }
        else
          variable_indices->insert(second_var->get_value(), Token_Index(0, 0, 1));
      }
    }
    else {
      node = Node_Filter_1::Create(data.network, data.job_queue, key);
      key = Node_Key_Symbol::Create(second);
    }

    if (third_var) {
      if (strlen(third_var->get_value()) != 0) {
        if (first_var && *first_var == *third_var) {
          node = Node_Filter_2::Create(data.network, data.job_queue, key, node);
          key = Node_Key_02::Create();
        }
        else if (second_var && *second_var == *third_var) {
          node = Node_Filter_2::Create(data.network, data.job_queue, key, node);
          key = Node_Key_12::Create();
        }
        else
          variable_indices->insert(third_var->get_value(), Token_Index(0, 0, 2));
      }
    }
    else {
      node = Node_Filter_2::Create(data.network, data.job_queue, key, node);
      key = Node_Key_Symbol::Create(third);
    }

    data.productions.top()->lhs.top().emplace(std::make_pair(node, key), variable_indices);

    if (data.productions.top()->lhs.top().size() == 2)
      data.join_conditions<Node_Join>();
  }

  template<typename Input>
  void Action<Condition_End>::apply(const Input &input, Data &data) {
    //std::cerr << "Condition_End: " << input.string() << std::endl;

    assert(data.symbols.size() == 1);
    data.symbols.pop_back();

    assert(data.productions.top()->lhs.top().size() == 1);
    const auto node = data.productions.top()->lhs.top().top();
    data.productions.top()->lhs.pop();
    data.productions.top()->lhs.top().emplace(node);
  }

  /// Left-Hand Side / LHS

  template<typename Input>
  void Action<Inner_Scope_Begin>::apply(const Input &input, Data &data) {
    //std::cerr << "Inner_Scope_Begin: " << input.string() << std::endl;

    data.productions.top()->lhs.emplace(std::stack<std::pair<std::pair<std::shared_ptr<Zeni::Rete::Node>, std::shared_ptr<const Node_Key>>, std::shared_ptr<Variable_Indices>>>());
  }

  template<typename Input>
  void Action<Inner_Scope_End>::apply(const Input &input, Data &data) {
    //std::cerr << "Inner_Scope_End: " << input.string() << std::endl;

    assert(data.productions.top()->lhs.top().size() == 1);
    auto top = data.productions.top()->lhs.top().top();
    data.productions.top()->lhs.pop();
    data.productions.top()->lhs.top().emplace(std::move(top));
  }

  template<typename Input>
  void Action<Join>::apply(const Input &input, Data &data) {
    //std::cerr << "Join: " << input.string() << std::endl;

    data.join_conditions<Node_Join>();
  }

  template<typename Input>
  void Action<Join_Existential>::apply(const Input &input, Data &data) {
    //std::cerr << "Join_Existential: " << input.string() << std::endl;

    data.join_conditions<Node_Join_Existential>();
  }

  template<typename Input>
  void Action<Join_Negation>::apply(const Input &input, Data &data) {
    //std::cerr << "Join_Negation: " << input.string() << std::endl;

    data.join_conditions<Node_Join_Negation>();
  }

  /// Right-Hand Side / RHS

  template<typename Input>
  void Action<Exit>::apply(const Input &, Data &data) {
    data.productions.top()->actions_or_retractions->push_back([](const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue>, const std::shared_ptr<Node_Action>, const std::shared_ptr<const Token>) {
      network->request_exit();
    });
  }

  template<typename Input>
  void Action<Make>::apply(const Input &input, Data &data) {
    //std::cerr << "Make: " << input.string() << std::endl;

    class Get_Symbol : public std::enable_shared_from_this<Get_Symbol> {
    public:
      virtual ~Get_Symbol() {}

      virtual std::shared_ptr<const Symbol> get(const Input &input, const std::shared_ptr<const Token>) const = 0;
    };

    class Get_Constant : public Get_Symbol {
    public:
      Get_Constant(const std::shared_ptr<const Symbol> &symbol) : m_symbol(symbol) {}

      std::shared_ptr<const Symbol> get(const Input &, const std::shared_ptr<const Token>) const override {
        return m_symbol;
      }

    private:
      const std::shared_ptr<const Symbol> m_symbol;
    };

    class Get_Variable : public Get_Symbol {
    public:
      Get_Variable(const Token_Index &token_index) : m_token_index(token_index) {}

      std::shared_ptr<const Symbol> get(const Input &input, const std::shared_ptr<const Token> token) const override {
        if (!token)
          throw parse_error("Parser error: cannot reference variables in top level actions", input);
        return (*token)[m_token_index];
      }

    private:
      const Token_Index m_token_index;
    };

    const auto to_Get_Symbol = [&input, &data](const auto &gen_symbol) {
      std::shared_ptr<const Get_Symbol> get_symbol;
      const auto symbol = gen_symbol->generate(data.productions.top()->substitutions);
      if (const auto variable = std::dynamic_pointer_cast<const Symbol_Variable>(symbol)) {
        if (strlen(variable->get_value()) == 0)
          throw parse_error("Parser error: cannot print the contents of an unnamed variable", input);

        const auto index = data.productions.top()->lhs.top().top().second->find_index(variable->get_value());
        if (index == Token_Index())
          throw parse_error("Parser error: variable used in the RHS not found in the LHS", input);

        get_symbol = std::make_shared<Get_Variable>(index);
      }
      else
        get_symbol = std::make_shared<Get_Constant>(symbol);
      return get_symbol;
    };

    std::vector<std::tuple<std::shared_ptr<const Get_Symbol>, std::shared_ptr<const Get_Symbol>, std::shared_ptr<const Get_Symbol>>> get_symbol;

    const auto symbol0 = data.symbols.front();
    data.symbols.pop_front();
    const std::shared_ptr<const Get_Symbol> get_symbol0 = to_Get_Symbol(symbol0);

    while (!data.symbols.empty()) {
      const auto symbol1 = data.symbols.front();
      data.symbols.pop_front();
      const std::shared_ptr<const Get_Symbol> get_symbol1 = to_Get_Symbol(symbol1);

      const auto symbol2 = data.symbols.front();
      data.symbols.pop_front();
      const std::shared_ptr<const Get_Symbol> get_symbol2 = to_Get_Symbol(symbol2);

      get_symbol.push_back(std::make_tuple(get_symbol0, get_symbol1, get_symbol2));
    }

    data.productions.top()->actions_or_retractions->push_back([&input, get_symbol](const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node_Action>, const std::shared_ptr<const Token> token) {
      for (const auto &getter : get_symbol)
        network->insert_wme(job_queue, std::make_shared<WME>(std::get<0>(getter)->get(input, token), std::get<1>(getter)->get(input, token), std::get<2>(getter)->get(input, token)));
    });
  }

  template<typename Input>
  void Action<Write>::apply(const Input &input, Data &data) {
    //std::cerr << "Write: " << input.string() << std::endl;

    class Write_Base : public std::enable_shared_from_this<Write_Base> {
    public:
      virtual ~Write_Base() {}

      virtual void write(std::ostream &os, const Input &input, const std::shared_ptr<const Token> token) const = 0;
    };

    class Write_String : public Write_Base {
    public:
      Write_String(const std::string &string) : m_string(string) {}

      void write(std::ostream &os, const Input &, const std::shared_ptr<const Token>) const override {
        os << m_string;
      }

    private:
      const std::string m_string;
    };

    class Write_Variable : public Write_Base {
    public:
      Write_Variable(const Token_Index &token_index) : m_token_index(token_index) {}

      void write(std::ostream &os, const Input &input, const std::shared_ptr<const Token> token) const override {
        if (!token)
          throw parse_error("Parser error: cannot reference variables in top level actions", input);
        (*token)[m_token_index]->print_contents(os);
      }

    private:
      const Token_Index m_token_index;
    };

    std::vector<std::shared_ptr<Write_Base>> writes;
    std::ostringstream oss;
    for (const auto gen_symbol : data.symbols) {
      const auto symbol = gen_symbol->generate(data.productions.top()->substitutions);
      if (const auto variable = std::dynamic_pointer_cast<const Symbol_Variable>(symbol)) {
        if (!oss.str().empty()) {
          writes.push_back(std::make_shared<Write_String>(oss.str()));
          oss.str("");
        }

        if (strlen(variable->get_value()) == 0)
          throw parse_error("Parser error: cannot print the contents of an unnamed variable", input);

        const auto index = data.productions.top()->lhs.top().top().second->find_index(variable->get_value());
        if (index == Token_Index())
          throw parse_error("Parser error: variable used in the RHS not found in the LHS", input);

        writes.push_back(std::make_shared<Write_Variable>(index));
      }
      else
        symbol->print_contents(oss);
    }
    if (!oss.str().empty()) {
      writes.push_back(std::make_shared<Write_String>(oss.str()));
      oss.str("");
    }
    data.symbols.clear();

    data.productions.top()->actions_or_retractions->push_back([&input, writes](const std::shared_ptr<Network>, const std::shared_ptr<Concurrency::Job_Queue>, const std::shared_ptr<Node_Action>, const std::shared_ptr<const Token> token) {
      std::ostringstream oss;
      for (const auto &write : writes)
        write->write(oss, input, token);
      std::cout << oss.str();
    });
  }

  /// Production Rules

  template<typename Input>
  void Action<Begin_Actions>::apply(const Input &input, Data &data) {
    data.productions.top()->actions_or_retractions = &data.productions.top()->actions;
  }

  template<typename Input>
  void Action<Begin_Retractions>::apply(const Input &input, Data &data) {
    data.productions.top()->actions_or_retractions = &data.productions.top()->retractions;
  }

  template<typename Input>
  void Action<Rule_Name>::apply(const Input &input, Data &data) {
    data.productions.top()->rule_name = input.string();
  }

  template<typename Input>
  void Action<Begin_Production>::apply(const Input &, Data &data) {
    data.productions.push(std::make_shared<Data::Production>(data.network, data.job_queue, false));
  }

  template<typename Input>
  void Action<Production>::apply(const Input &input, Data &data) {
    //std::cerr << "Production: " << input.string() << std::endl;

    assert(data.productions.top()->lhs.size() == 1);
    assert(data.productions.top()->lhs.top().size() == 1);
    const auto node = data.productions.top()->lhs.top().top();
    data.productions.top()->lhs.top().pop();

    class Input_Node_Wrapper {
      Input_Node_Wrapper(const Input_Node_Wrapper &) = delete;
      Input_Node_Wrapper & operator=(const Input_Node_Wrapper &) = delete;

    public:
      Input_Node_Wrapper(const std::shared_ptr<Network> network_, const std::shared_ptr<Node> input_, const std::shared_ptr<const Node_Key> node_key_, const std::shared_ptr<const Variable_Indices> variable_indices_)
        : m_network(network_), m_input(input_), node_key(node_key_), variable_indices(variable_indices_)
      {
      }

      ~Input_Node_Wrapper() {
        /// Must be done this way since the Worker Threads could be swapped out before destruction
        m_input->send_disconnect_from_parents(m_network, m_network->get_Worker_Threads()->get_main_Job_Queue());
      }

      std::shared_ptr<Node> get_input() const {
        m_input->connect_to_parents_again(m_network, m_network->get_Worker_Threads()->get_main_Job_Queue());
        return m_input;
      }

    private:
      const std::shared_ptr<Network> m_network;
      const std::shared_ptr<Node> m_input;

    public:
      const std::shared_ptr<const Node_Key> node_key;
      const std::shared_ptr<const Variable_Indices> variable_indices;
    };

    const auto input_node = std::make_shared<Input_Node_Wrapper>(data.network, node.first.first, node.first.second, node.second);

    const std::string rule_name = data.productions.top()->rule_name;

    std::shared_ptr<decltype(Data::Production::actions)> actions =
      std::make_shared<decltype(Data::Production::actions)>(std::move(data.productions.top()->actions));
    data.productions.top()->actions.clear();

    std::shared_ptr<decltype(Data::Production::actions)> retractions =
      std::make_shared<decltype(Data::Production::actions)>(std::move(data.productions.top()->retractions));
    data.productions.top()->retractions.clear();

    data.productions.pop();

    const bool user_command = data.productions.top()->user_command;

    data.productions.top()->actions_or_retractions->push_back([input_node, rule_name, user_command, actions, retractions](const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node_Action>, const std::shared_ptr<const Token> token) {
      Zeni::Rete::Node_Action::Create(network, job_queue, rule_name, user_command, input_node->node_key, input_node->get_input(), input_node->variable_indices,
        [actions](const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token)
      {
        //std::cout << rete_action.get_name() << " +: " << token << std::endl;
        for (const auto &action : *actions)
          action(network, job_queue, rete_action, token);
      }, [retractions](const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Token> token)
      {
        //std::cout << rete_action.get_name() << " -: " << token << std::endl;
        for (const auto &retraction : *retractions)
          retraction(network, job_queue, rete_action, token);
      });
    });
  }

  /// Overarching grammar

  template<typename Input>
  void Action<Command>::apply(const Input &input, Data &data) {
    assert(data.productions.top()->actions.size() == 1);
    data.productions.top()->actions.front()(data.network, data.job_queue, nullptr, nullptr);
    data.productions.top()->actions.pop_front();
  }

}

#endif
