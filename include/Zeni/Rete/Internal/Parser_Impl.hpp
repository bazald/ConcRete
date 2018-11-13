#ifndef ZENI_RETE_PARSER_IMPL_HPP
#define ZENI_RETE_PARSER_IMPL_HPP

#include "Zeni/Concurrency/Worker_Threads.hpp"
#include "../Network.hpp"
#include "../Node_Action.hpp"
#include "../Node_Filter_1.hpp"
#include "../Node_Filter_2.hpp"
#include "../Node_Join.hpp"
#include "../Node_Join_Existential.hpp"
#include "../Node_Join_Negation.hpp"
#include "../Node_Key.hpp"
#include "../Parser.hpp"
#include "../Symbol.hpp"
#include "../Variable_Indices.hpp"

#include <deque>
#include <list>
#include <stack>
#include <queue>

#define TAO_PEGTL_NAMESPACE Zeni_Rete_PEG

#include "tao/pegtl.hpp"
#include "tao/pegtl/analyze.hpp"

namespace Zeni::Rete::PEG {

  using namespace tao::Zeni_Rete_PEG;

  struct Data {
    class Production {
      Production(const Production &) = delete;
      Production & operator=(const Production &) = delete;

    public:
      Production(const std::shared_ptr<Network> network_, const std::shared_ptr<Concurrency::Job_Queue> job_queue_, const bool user_command_)
        : network(network_), job_queue(job_queue_), user_command(user_command_)
      {
        lhs.push(decltype(lhs)::value_type());
      }

      ~Production() {
        while (!lhs.empty()) {
          while (!lhs.top().empty()) {
            lhs.top().top().first.first->send_disconnect_from_parents(network, job_queue);
            lhs.top().pop();
          }
          lhs.pop();
        }
      }

      const std::shared_ptr<Network> network;
      const std::shared_ptr<Concurrency::Job_Queue> job_queue;

      const bool user_command;

      std::string rule_name;
      std::stack<std::stack<std::pair<std::pair<std::shared_ptr<Zeni::Rete::Node>, std::shared_ptr<const Node_Key>>, std::shared_ptr<Variable_Indices>>>> lhs;
      std::list<Node_Action::Action> actions;
      std::list<Node_Action::Action> retractions;
      std::list<Node_Action::Action> * actions_or_retractions = &actions;
    };

    Data(const std::shared_ptr<Network> network_, const std::shared_ptr<Concurrency::Job_Queue> job_queue_, const bool user_command_)
      : network(network_), job_queue(job_queue_)
    {
      productions.push(std::make_shared<Production>(network_, job_queue_, user_command_));
    }

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

  template <typename Rule>
  struct Error : public normal<Rule>
  {
    static inline const char * error_message();

    template <typename Input>
    static void raise(const Input &input, Data &) {
      throw parse_error(error_message(), input);
    }
  };

  struct plus_minus : one<'+', '-'> {};
  struct dot : one<'.'> {};

  struct inf : seq<istring<'i', 'n', 'f'>, opt<istring<'i', 'n', 'i', 't', 'y'>>> {};
  struct nan : seq<istring<'n', 'a', 'n'>, opt<one<'('>, plus<alnum>, one<')'>>> {};

  template<typename D>
  struct number : if_then_else<dot, plus<D>, seq<plus<D>, opt<dot, star<D>>>> {};

  struct e : one<'e', 'E'> {};
  struct p : one<'p', 'P'> {};
  struct exponent : seq<opt<plus_minus>, plus<digit>> {};

  struct decimal : seq<number<digit>, opt<e, exponent>> {};

  struct comment : if_must<one<'#'>, until<eolf>> {};
  template<> inline const char * Error<until<eolf>>::error_message() { return "Parser error: failed to process comment"; } ///< Probably cannot be triggered

  struct space_comment : sor<space, comment> {};

  struct Unquoted_String : seq<plus<alpha>, star<sor<alnum, one<'-', '_', '*'>>>> {};

  struct Constant_Float : if_then_else<plus_minus, must<sor<decimal, inf, nan>>, sor<decimal, inf, nan>> {};
  template<> inline const char * Error<sor<decimal, inf, nan>>::error_message() { return "Parser error: '+'/'-' must be immediately followed by a numerical value to make a valid symbol"; }
  struct Constant_Int : if_then_else<plus_minus, must<plus<digit>>, plus<digit>> {};
  template<> inline const char * Error<plus<digit>>::error_message() { return "Parser error: '+'/'-' must be immediately followed by a numerical value to make a valid symbol"; }
  struct Constant_String : Unquoted_String {};
  struct Quoted_Constant_String : if_must<one<'|'>, seq<plus<not_one<'|'>>, one<'|'>>> {};
  template<> inline const char * Error<seq<plus<not_one<'|'>>, one<'|'>>>::error_message() { return "Parser error: quoted string constant not closed (mismatched '|'s?)"; }
  struct CRLF : seq<one<'('>, star<space_comment>, string<'c', 'r', 'l', 'f'>, star<space_comment>, one<')'>> {};
  struct Variable : if_must<one<'<'>, seq<Unquoted_String, one<'>'>>> {};
  template<> inline const char * Error<seq<Unquoted_String, one<'>'>>>::error_message() { return "Parser error: variable not closed (mismatched '<>'s?)"; }
  struct Unnamed_Variable : seq<one<'{'>, star<space_comment>, one<'}'>> {};

  struct Symbol_w_Var : sor<seq<at<minus<Constant_Float, Constant_Int>>, Constant_Float>, Constant_Int, Constant_String, Quoted_Constant_String, CRLF, Variable, Unnamed_Variable> {};
  struct Symbol_wo_Var : sor<seq<at<minus<Constant_Float, Constant_Int>>, Constant_Float>, Constant_Int, Constant_String, Quoted_Constant_String, CRLF> {};

  struct Condition_Attr_Value : seq<plus<space_comment>, one<'^'>, star<space_comment>, Symbol_w_Var, plus<space_comment>, Symbol_w_Var> {};
  struct WME_Attr_Value : seq<plus<space_comment>, one<'^'>, star<space_comment>, Symbol_wo_Var, plus<space_comment>, Symbol_wo_Var> {};

  struct Inner_Condition : seq<star<space_comment>, Symbol_w_Var, plus<Condition_Attr_Value>> {};
  struct Condition_Body : seq<star<space_comment>, Inner_Condition, star<space_comment>, one<')'>, star<space_comment>> {};
  struct Condition_End : if_must<one<'('>, Condition_Body> {};
  template<> inline const char * Error<Condition_Body>::error_message() { return "Parser error: invalid condition syntax (mismatched '()'s?)"; }
  struct Condition_Begin : at<Condition_End> {};
  struct Condition : seq<Condition_Begin, Condition_End> {};

  struct Inner_WME : seq<star<space_comment>, Symbol_wo_Var, plus<WME_Attr_Value>> {};

  struct Subnode_First;
  struct Subnode_Rest;

  struct Inner_Scope_Body : seq<Subnode_First, star<space_comment>, star<Subnode_Rest, star<space_comment>>> {};
  struct Inner_Scope_End : must<Inner_Scope_Body> {};
  template<> inline const char * Error<Inner_Scope_Body>::error_message() { return "Parser error: invalid left-hand side for production rule"; }
  struct Inner_Scope_Begin : at<Inner_Scope_End> {};
  struct Inner_Scope : seq<Inner_Scope_Begin, Inner_Scope_End> {};
  struct Outer_Scope_Body : seq<Inner_Scope, one<'}'>, star<space_comment>> {};
  struct Outer_Scope : if_must<one<'{'>, Outer_Scope_Body> {};
  template<> inline const char * Error<Outer_Scope_Body>::error_message() { return "Parser error: invalid scope syntax (mismatched '{}'s?)"; }
  struct Scope : seq<sor<Outer_Scope, Condition>, star<space_comment>> {};

  struct Join : Scope {};
  struct EScope : Scope {};
  struct NScope : Scope {};
  struct Join_Existential : if_must<one<'+'>, EScope> {};
  template<> inline const char * Error<EScope>::error_message() { return "Parser error: '+' must be immediately followed by a valid condition or scope"; }
  struct Join_Negation : if_must<seq<not_at<string<'-', '-', '>'>>, one<'-'>>, NScope> {};
  template<> inline const char * Error<NScope>::error_message() { return "Parser error: '-' must be immediately followed by a valid condition or scope"; }

  struct Subnode_First : Scope {};
  struct Subnode_Rest : sor<Join, Join_Existential, Join_Negation> {};

  struct Rule_Name : Unquoted_String {};
  struct Actions;
  struct Retractions;
  struct Inner_Production_Body :
    seq<Rule_Name, star<space_comment>,
    Inner_Scope, star<space_comment>,
    Actions, star<space_comment>,
    opt<seq<Retractions, star<space_comment>>>> {};

  struct Exit : string<'e', 'x', 'i', 't'> {};

  struct Inner_Make : seq<plus<plus<space_comment>, Inner_WME>> {};
  struct Make : if_must<string<'m', 'a', 'k', 'e'>, Inner_Make> {};
  template<> inline const char * Error<Inner_Make>::error_message() { return "Parser error: 'make' must be followed by a valid WME"; }

  struct Production_Body : seq<plus<space_comment>, Inner_Production_Body> {};
  struct End_Production : if_must<one<'p'>, Production_Body> {};
  template<> inline const char * Error<Production_Body>::error_message() { return "Parser error: 'p' must be followed by a valid production rule body"; }
  struct Begin_Production : at<End_Production> {};
  struct Production : seq<Begin_Production, End_Production> {};

  struct Inner_Write : plus<plus<space_comment>, Symbol_w_Var> {};
  struct Write : if_must<string<'w', 'r', 'i', 't', 'e'>, Inner_Write> {};
  template<> inline const char * Error<Inner_Write>::error_message() { return "Parser error: 'write' must be followed by valid Symbols"; }

  struct Inner_Action : sor<Exit, Make, Production, Write> {};
  struct Enclosed_Action : seq<one<'('>, star<space_comment>, Inner_Action, star<space_comment>, one<')'>, star<space_comment>> {};
  struct Action_List : plus<Enclosed_Action> {};
  struct Inner_Actions : seq<string<'-', '-', '>'>, star<space_comment>, Action_List> {};
  struct Begin_Actions : at<Inner_Actions> {};
  struct Actions : seq<Begin_Actions, Inner_Actions> {};
  struct Inner_Retractions : seq<string<'<', '-', '-'>, star<space_comment>, Action_List> {};
  struct Begin_Retractions : at<Inner_Retractions> {};
  struct Retractions : seq<Begin_Retractions, Inner_Retractions> {};

  struct Command : Enclosed_Action {};

  struct ConcRete_Grammar : seq<star<space_comment>, star<seq<Command, star<space_comment>>>, eof> {};

  struct ConcRete : must<ConcRete_Grammar> {};
  template<> inline const char * Error<ConcRete_Grammar>::error_message() { return "Parser error: could not process ConcRete grammar"; }

  template <typename Rule>
  struct Action : nothing<Rule> {};

  template <>
  struct Action<Constant_Float> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      const double d = std::stod(input.string());
      //std::cerr << "Constant_Float: " << d << std::endl;
      data.symbols.emplace_back(input.string(), std::make_shared<Symbol_Constant_Float>(d));
    }
  };

  template <>
  struct Action<Constant_Int> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      const int64_t i = std::stoll(input.string());
      //std::cerr << "Constant_Int: " << i << std::endl;
      data.symbols.emplace_back(input.string(), std::make_shared<Symbol_Constant_Int>(i));
    }
  };

  template <>
  struct Action<Constant_String> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cerr << "Constant_String: " << input.string() << std::endl;
      data.symbols.emplace_back(input.string(), std::make_shared<Symbol_Constant_String>(input.string()));
    }
  };

  template <>
  struct Action<Quoted_Constant_String> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cerr << "Quoted_Constant_String: " << input.string() << std::endl;
      assert(input.string().size() > 2);
      const auto substr = input.string().substr(1, input.string().size() - 2);
      data.symbols.emplace_back(substr, std::make_shared<Symbol_Constant_String>(substr));
    }
  };

  template <>
  struct Action<CRLF> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cerr << "CRLF: " << input.string() << std::endl;
      data.symbols.emplace_back("\r\n", std::make_shared<Symbol_Constant_String>("\r\n"));
    }
  };

  template <>
  struct Action<Variable> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cerr << "Constant_Variable: " << input.string() << std::endl;
      assert(input.string().size() > 2);
      const auto substr = input.string().substr(1, input.string().size() - 2);
      data.symbols.emplace_back(substr, nullptr);
    }
  };

  template <>
  struct Action<Unnamed_Variable> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cerr << "Unnamed_Variable: " << input.string() << std::endl;
      data.symbols.emplace_back("", nullptr);
    }
  };

  template <>
  struct Action<Rule_Name> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      data.productions.top()->rule_name = input.string();
    }
  };

  template <>
  struct Action<Condition_Begin> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cerr << "Condition_Begin: " << input.string() << std::endl;

      data.productions.top()->lhs.emplace(std::stack<std::pair<std::pair<std::shared_ptr<Zeni::Rete::Node>, std::shared_ptr<const Node_Key>>, std::shared_ptr<Variable_Indices>>>());
    }
  };

  template <>
  struct Action<Condition_Attr_Value> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cerr << "Condition_Attr_Value: " << input.string() << std::endl;

      assert(data.symbols.size() == 3);
      const auto third = data.symbols.back();
      data.symbols.pop_back();
      const auto second = data.symbols.back();
      data.symbols.pop_back();
      const auto first = data.symbols.back();
      // Deliberately leave the first symbol on the stack

      std::shared_ptr<Zeni::Rete::Node> node = data.network;
      auto variable_indices = Variable_Indices::Create();

      std::shared_ptr<const Node_Key> key;
      if (first.second)
        key = Node_Key_Symbol::Create(first.second);
      else {
        key = Node_Key_Null::Create();
        if (!first.first.empty())
          variable_indices->insert(first.first, Token_Index(0, 0, Symbol_Variable::First));
      }

      if (second.second) {
        node = Node_Filter_1::Create(data.network, data.job_queue, key);
        key = Node_Key_Symbol::Create(second.second);
      }
      else {
        if (!second.first.empty()) {
          if (!first.second && first.first == second.first) {
            node = Node_Filter_1::Create(data.network, data.job_queue, key);
            key = Node_Key_01::Create();
          }
          else
            variable_indices->insert(second.first, Token_Index(0, 0, Symbol_Variable::Second));
        }
      }

      if (third.second) {
        node = Node_Filter_2::Create(data.network, data.job_queue, key, node);
        key = Node_Key_Symbol::Create(third.second);
      }
      else {
        if (!third.first.empty()) {
          if (!first.second && first.first == third.first) {
            node = Node_Filter_2::Create(data.network, data.job_queue, key, node);
            key = Node_Key_02::Create();
          }
          else if (!second.second && second.first == third.first) {
            node = Node_Filter_2::Create(data.network, data.job_queue, key, node);
            key = Node_Key_12::Create();
          }
          else
            variable_indices->insert(third.first, Token_Index(0, 0, Symbol_Variable::Third));
        }
      }

      data.productions.top()->lhs.top().emplace(std::make_pair(node, key), variable_indices);

      if (data.productions.top()->lhs.top().size() == 2)
        data.join_conditions<Node_Join>();
    }
  };

  template <>
  struct Action<Condition_End> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cerr << "Condition_End: " << input.string() << std::endl;

      assert(data.symbols.size() == 1);
      data.symbols.pop_back();

      assert(data.productions.top()->lhs.top().size() == 1);
      const auto node = data.productions.top()->lhs.top().top();
      data.productions.top()->lhs.pop();
      data.productions.top()->lhs.top().emplace(node);
    }
  };

  template <>
  struct Action<Inner_Scope_Begin> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cerr << "Inner_Scope_Begin: " << input.string() << std::endl;

      data.productions.top()->lhs.emplace(std::stack<std::pair<std::pair<std::shared_ptr<Zeni::Rete::Node>, std::shared_ptr<const Node_Key>>, std::shared_ptr<Variable_Indices>>>());
    }
  };

  template <>
  struct Action<Inner_Scope_End> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cerr << "Inner_Scope_End: " << input.string() << std::endl;

      assert(data.productions.top()->lhs.top().size() == 1);
      auto top = data.productions.top()->lhs.top().top();
      data.productions.top()->lhs.pop();
      data.productions.top()->lhs.top().emplace(std::move(top));
    }
  };

  template <>
  struct Action<Join> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cerr << "Join: " << input.string() << std::endl;

      data.join_conditions<Node_Join>();
    }
  };

  template <>
  struct Action<Join_Existential> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cerr << "Join_Existential: " << input.string() << std::endl;

      data.join_conditions<Node_Join_Existential>();
    }
  };

  template <>
  struct Action<Join_Negation> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cerr << "Join_Negation: " << input.string() << std::endl;

      data.join_conditions<Node_Join_Negation>();
    }
  };

  template <>
  struct Action<Exit> {
    template<typename Input>
    static void apply(const Input &, Data &) {
      throw Parser::Exit();
    }
  };

  template <>
  struct Action<Make> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
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

      const auto to_Get_Symbol = [&input,&data](const auto &varname_symbol) {
        std::shared_ptr<const Get_Symbol> get_symbol;
        if (varname_symbol.second)
          get_symbol = std::make_shared<Get_Constant>(varname_symbol.second);
        else {
          if (varname_symbol.first.empty())
            throw parse_error("Parser error: cannot print the contents of an unnamed variable", input);

          const auto index = data.productions.top()->lhs.top().top().second->find_index(varname_symbol.first);
          if (index == Token_Index())
            throw parse_error("Parser error: variable used in the RHS not found in the LHS", input);

          get_symbol = std::make_shared<Get_Variable>(index);
        }
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

      data.productions.top()->actions_or_retractions->push_back([&input,get_symbol](const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node_Action>, const std::shared_ptr<const Token> token) {
        for (const auto &getter : get_symbol)
          network ->insert_wme(job_queue, std::make_shared<WME>(std::get<0>(getter)->get(input, token), std::get<1>(getter)->get(input, token), std::get<2>(getter)->get(input, token)));
      });
    }
  };

  template <>
  struct Action<Begin_Production> {
    template<typename Input>
    static void apply(const Input &, Data &data) {
      data.productions.push(std::make_shared<Data::Production>(data.network, data.job_queue, false));
    }
  };

  template <>
  struct Action<Production> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cerr << "Source_Production: " << input.string() << std::endl;

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

        const std::shared_ptr<const Node_Key> node_key;
        const std::shared_ptr<const Variable_Indices> variable_indices;

      private:
        const std::shared_ptr<Network> m_network;
        const std::shared_ptr<Node> m_input;
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

      data.productions.top()->actions_or_retractions->push_back([input_node,rule_name,user_command,actions,retractions](const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node_Action>, const std::shared_ptr<const Token> token) {
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
  };

  template <>
  struct Action<Write> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
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
      for (const auto symbol : data.symbols) {
        if (!symbol.second) {
          if (!oss.str().empty()) {
            writes.push_back(std::make_shared<Write_String>(oss.str()));
            oss.str("");
          }

          if (symbol.first.empty())
            throw parse_error("Parser error: cannot print the contents of an unnamed variable", input);

          const auto index = data.productions.top()->lhs.top().top().second->find_index(symbol.first);
          if (index == Token_Index())
            throw parse_error("Parser error: variable used in the RHS not found in the LHS", input);

          writes.push_back(std::make_shared<Write_Variable>(index));
        }
        else
          symbol.second->print_contents(oss);
      }
      if (!oss.str().empty()) {
        writes.push_back(std::make_shared<Write_String>(oss.str()));
        oss.str("");
      }
      data.symbols.clear();

      data.productions.top()->actions_or_retractions->push_back([&input,writes](const std::shared_ptr<Network>, const std::shared_ptr<Concurrency::Job_Queue>, const std::shared_ptr<Node_Action>, const std::shared_ptr<const Token> token) {
        std::ostringstream oss;
        for (const auto &write : writes)
          write->write(oss, input, token);
        std::cout << oss.str();
      });
    }
  };

  template <>
  struct Action<Begin_Actions> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      data.productions.top()->actions_or_retractions = &data.productions.top()->actions;
    }
  };

  template <>
  struct Action<Begin_Retractions> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      data.productions.top()->actions_or_retractions = &data.productions.top()->retractions;
    }
  };

  template <>
  struct Action<Command> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      assert(data.productions.top()->actions.size() == 1);
      data.productions.top()->actions.front()(data.network, data.job_queue, nullptr, nullptr);
      data.productions.top()->actions.pop_front();
    }
  };

}

namespace Zeni::Rete {

  class Parser_Analyzer {
  private:
    Parser_Analyzer() {
      [[maybe_unused]] const size_t number_of_issues = PEG::analyze<PEG::ConcRete>();
      assert(number_of_issues == 0);
    }

  public:
    static Parser_Analyzer & get() {
      static Parser_Analyzer parser_analyzer;
      return parser_analyzer;
    }
  };

  class Parser_Impl : public Parser {
    Parser_Impl(const Parser_Impl &) = delete;
    Parser_Impl & operator=(const Parser_Impl &) = delete;

    template <typename Input>
    void parse(Input &input, const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_command) {
      PEG::Data data(network, job_queue, user_command);

      try {
        PEG::parse<PEG::ConcRete, PEG::Action, PEG::Error>(input, data);
      }
      catch (const PEG::parse_error &error) {
        std::ostringstream oposition;
        oposition << error.positions.front();
        const std::string_view what = error.what();
        std::cerr << what.substr(oposition.str().length() + 2) << " at line " << error.positions.front().line << ", column " << error.positions.front().byte_in_line << std::endl;
      }
    }

  protected:
    Parser_Impl();

  public:
    ZENI_RETE_LINKAGE static std::shared_ptr<Parser_Impl> Create();

    void parse_file(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::string &filename, const bool user_command) override;

    void parse_string(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::string_view str, const bool user_command) override;
  };

}

#endif
