#ifndef ZENI_RETE_PARSER_IMPL_HPP
#define ZENI_RETE_PARSER_IMPL_HPP

#include "../Network.hpp"
#include "../Node_Action.hpp"
#include "../Node_Filter_1.hpp"
#include "../Node_Filter_2.hpp"
#include "../Node_Join.hpp"
#include "../Node_Key.hpp"
#include "../Parser.hpp"
#include "../Symbol.hpp"
#include "../Variable_Indices.hpp"

//#include <cstdio>
//#include <iostream>
//#include <stack>
#include <queue>
//#include <string_view>

#define TAO_PEGTL_NAMESPACE Zeni_Rete_PEG

#include "tao/pegtl.hpp"
#include "tao/pegtl/analyze.hpp"

namespace Zeni::Rete::PEG {

  using namespace tao::Zeni_Rete_PEG;

  struct plus_minus : opt<one<'+', '-'>> {};
  struct dot : one<'.'> {};

  struct inf : seq<istring<'i', 'n', 'f'>, opt<istring<'i', 'n', 'i', 't', 'y'>>> {};
  struct nan : seq<istring<'n', 'a', 'n'>, opt<one<'('>, plus<alnum>, one<')'>>> {};

  template<typename D>
  struct number : if_then_else<dot, plus<D>, seq<plus<D>, opt<dot, star<D>>>> {};

  struct e : one<'e', 'E'> {};
  struct p : one<'p', 'P'> {};
  struct exponent : seq<plus_minus, plus<digit>> {};

  struct decimal : seq<number<digit>, opt<e, exponent>> {};

  struct at_space_eof : at<sor<space, eof>> {};

  struct Constant_Float : seq<plus_minus, sor<decimal, inf, nan>> {};
  struct Constant_Int : seq<plus_minus, plus<digit>> {};
  struct Constant_String : seq<plus<alpha>, star<sor<alnum, one<'-', '_', '*'>>>> {};
  struct Variable : seq<one<'<'>, plus<alpha>, star<sor<alnum, one<'-', '_', '*'>>>, one<'>'>> {};

  struct Symbol : sor<seq<at<minus<Constant_Float, Constant_Int>>, Constant_Float>, Constant_Int, Constant_String, Variable> {};

  struct comment : if_must<one<'#'>, until<eolf>> {};
  struct space_comment : sor<space, comment> {};

  struct Condition : seq<one<'('>, star<space_comment>, Symbol, star<space_comment>, one<'^'>, Symbol, plus<space_comment>, Symbol, star<space_comment>, one<')'>> {};

  struct Conditions : list_tail<Condition, star<space_comment>> {};

  struct Rule_Name : seq<plus<alpha>, star<sor<alnum, one<'-', '_', '*'>>>> {};
  struct Source_Production : seq<string<'s', 'p'>, star<space_comment>, one<'{'>, star<space_comment>,
    Rule_Name, star<space_comment>,
    Conditions,
    string<'-', '-', '>'>, star<space_comment>,
    one<'}'>> {};

  struct Grammar : must<seq<star<space_comment>, list_tail<Source_Production, star<space_comment>>>, eof> {};

  struct Data {
    Data(const std::shared_ptr<Network> network_, const std::shared_ptr<Concurrency::Job_Queue> job_queue_, const bool user_command_) : network(network_), job_queue(job_queue_), user_command(user_command_) {}

    const std::shared_ptr<Network> network;
    const std::shared_ptr<Concurrency::Job_Queue> job_queue;
    const bool user_command;
    std::queue<std::pair<std::string, std::shared_ptr<Rete::Symbol>>> symbols;
    std::queue<std::pair<std::pair<std::shared_ptr<Node>, std::shared_ptr<Node_Key>>, std::shared_ptr<Variable_Indices>>> filters;
    std::queue<std::pair<std::pair<std::shared_ptr<Node>, std::shared_ptr<Node_Key>>, std::shared_ptr<Variable_Indices>>> nodes;
    std::string rule_name;
  };

  template <typename Rule>
  struct Action : nothing<Rule> {};

  template <>
  struct Action<Constant_Float> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      const double d = std::stod(input.string());
      //std::cout << "Double: " << d << std::endl;
      data.symbols.emplace(input.string(), std::make_shared<Symbol_Constant_Float>(d));
    }
  };

  template <>
  struct Action<Constant_Int> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      const int64_t i = std::stoll(input.string());
      //std::cout << "Int: " << i << std::endl;
      data.symbols.emplace(input.string(), std::make_shared<Symbol_Constant_Int>(i));
    }
  };

  template <>
  struct Action<Constant_String> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cout << "String: " << input.string() << std::endl;
      data.symbols.emplace(input.string(), std::make_shared<Symbol_Constant_String>(input.string()));
    }
  };

  template <>
  struct Action<Variable> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      std::string str = input.string();
      str = str.substr(1, str.length() - 2);
      //std::cout << "Variable: " << str << std::endl;
      data.symbols.emplace(input.string(), nullptr);
    }
  };

  template <>
  struct Action<Rule_Name> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      data.rule_name = input.string();
    }
  };

  template <>
  struct Action<Condition> {
    template<typename Input>
    static void apply(const Input &, Data &data) {
      assert(data.symbols.size() == 3);
      const std::pair<std::string, std::shared_ptr<Rete::Symbol>> first = data.symbols.front();
      data.symbols.pop();
      auto second = data.symbols.front();
      data.symbols.pop();
      auto third = data.symbols.front();
      data.symbols.pop();

      std::shared_ptr<Node> node = data.network;
      auto variable_indices = Variable_Indices::Create();

      std::shared_ptr<Node_Key> key;
      if (first.second)
        key = std::make_shared<Node_Key_Symbol>(first.second);
      else {
        key = std::make_shared<Node_Key_Null>();
        variable_indices->insert(first.first, Token_Index(0, 0, Symbol_Variable::First));
      }

      if (second.second) {
        node = Node_Filter_1::Create(data.network, data.job_queue, key);
        key = std::make_shared<Node_Key_Symbol>(second.second);
      }
      else {
        if (!first.second && first.first == second.first) {
          node = Node_Filter_1::Create(data.network, data.job_queue, key);
          key = std::make_shared<Node_Key_01>();
        }
        else
          variable_indices->insert(second.first, Token_Index(0, 0, Symbol_Variable::Second));
      }

      if (third.second) {
        node = Node_Filter_2::Create(data.network, data.job_queue, key, node);
        key = std::make_shared<Node_Key_Symbol>(third.second);
      }
      else {
        if (!first.second && first.first == third.first) {
          node = Node_Filter_2::Create(data.network, data.job_queue, key, node);
          key = std::make_shared<Node_Key_02>();
        }
        else if (!second.second && second.first == third.first) {
          node = Node_Filter_2::Create(data.network, data.job_queue, key, node);
          key = std::make_shared<Node_Key_12>();
        }
        else
          variable_indices->insert(third.first, Token_Index(0, 0, Symbol_Variable::Third));
      }

      //std::cout << "Condition: " << input.string() << std::endl;

      data.filters.emplace(std::make_pair(node, key), variable_indices);
    }
  };

  template <>
  struct Action<Conditions> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cout << "Conditions: " << input.string() << std::endl;

      assert(!data.filters.empty());
      std::pair<std::pair<std::shared_ptr<Node>, std::shared_ptr<Node_Key>>, std::shared_ptr<Variable_Indices>> first = data.filters.front();
      data.filters.pop();

      while (!data.filters.empty()) {
        auto second = data.filters.front();
        data.filters.pop();
        Variable_Bindings variable_bindings;
        for (auto right : second.second->get_indices()) {
          auto left = first.second->find_index(right.first);
          if (left != Token_Index())
            variable_bindings.emplace(left, right.second);
        }
        first.second = Variable_Indices::Create(first.first.first->get_size(), first.first.first->get_token_size(), *first.second, *second.second);
        first.first = std::make_pair(Node_Join::Create(data.network, data.job_queue, first.first.second, second.first.second, first.first.first, second.first.first, std::move(variable_bindings)), std::make_shared<Node_Key_Null>());
      }

      data.nodes.emplace(first);
    }
  };

  template <>
  struct Action<Source_Production> {
    template<typename Input>
    static void apply(const Input &, Data &data) {
      //std::cout << "Source_Production: " << input.string() << std::endl;

      assert(data.filters.empty());
      assert(data.nodes.size() == 1);
      Zeni::Rete::Node_Action::Create(data.network, data.job_queue, data.rule_name, data.user_command, data.nodes.front().first.second, data.nodes.front().first.first, data.nodes.front().second,
        [](const Zeni::Rete::Node_Action &rete_action, const Zeni::Rete::Token &token) {
        std::cout << rete_action.get_name() << " +: " << token << std::endl;
      }, [](const Zeni::Rete::Node_Action &rete_action, const Zeni::Rete::Token &token) {
        std::cout << rete_action.get_name() << " -: " << token << std::endl;
      });
      data.nodes.pop();
    }
  };

}

namespace Zeni::Rete {

  class Parser_Analyzer {
  private:
    Parser_Analyzer() {
      PEG::analyze<PEG::Grammar>();
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

      PEG::parse<PEG::Grammar, PEG::Action>(input, data);
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
