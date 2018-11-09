#ifndef ZENI_RETE_PARSER_IMPL_HPP
#define ZENI_RETE_PARSER_IMPL_HPP

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

#include <stack>

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

  struct Condition_Attr_Value : seq<one<'^'>, Symbol, plus<space_comment>, Symbol> {};
  struct Condition_End : seq<one<'('>, star<space_comment>, Symbol, star<space_comment>, plus<Condition_Attr_Value, star<space_comment>>, one<')'>> {};
  struct Condition_Begin : at<Condition_End> {};
  struct Condition : seq<Condition_Begin, Condition_End> {};

  struct Subnode_First;
  struct Subnode_Rest;

  struct Inner_Scope_End : seq<Subnode_First, star<star<space_comment>, Subnode_Rest>> {};
  struct Inner_Scope_Begin : at<Inner_Scope_End> {};
  struct Inner_Scope : seq<Inner_Scope_Begin, Inner_Scope_End> {};

  struct Outer_Scope : seq<one<'{'>, star<space_comment>, Inner_Scope, star<space_comment>, one<'}'>> {};
  struct Scope : sor<Outer_Scope, Condition> {};
  struct Scope_Existential : seq<one<'+'>, sor<Outer_Scope, Condition>> {};
  struct Scope_Negation : seq<one<'-'>, sor<Outer_Scope, Condition>> {};

  struct Subnode_First : Scope {};
  struct Subnode_Rest : sor<Scope, Scope_Existential, Scope_Negation> {};

  struct Rule_Name : seq<plus<alpha>, star<sor<alnum, one<'-', '_', '*'>>>> {};
  struct Source_Production : seq<string<'s', 'p'>, star<space_comment>, one<'{'>, star<space_comment>,
    Rule_Name, star<space_comment>,
    Inner_Scope, star<space_comment>,
    string<'-', '-', '>'>, star<space_comment>,
    one<'}'>> {};

  struct Grammar : must<seq<star<space_comment>, list_tail<Source_Production, star<space_comment>>>, eof> {};

  struct Data {
    Data(const std::shared_ptr<Network> network_, const std::shared_ptr<Concurrency::Job_Queue> job_queue_, const bool user_command_)
      : network(network_), job_queue(job_queue_), user_command(user_command_)
    {
      nodes.push(std::stack<std::pair<std::pair<std::shared_ptr<Zeni::Rete::Node>, std::shared_ptr<const Node_Key>>, std::shared_ptr<Variable_Indices>>>());
    }

    template <typename Join_Type>
    void join_conditions() {
      const auto node_right = nodes.top().top();
      nodes.top().pop();
      const auto node_left = nodes.top().top();
      nodes.top().pop();

      Variable_Bindings variable_bindings;
      for (auto right : node_right.second->get_indices()) {
        auto left = node_left.second->find_index(right.first);
        if (left != Token_Index())
          variable_bindings.emplace(left, right.second);
      }

      const auto node = Join_Type::Create(network, job_queue, node_left.first.second, node_right.first.second, node_left.first.first, node_right.first.first, std::move(variable_bindings));
      const auto variable_indices = Variable_Indices::Create(node_left.first.first->get_size(), node_left.first.first->get_token_size(), *node_left.second, *node_right.second);

      nodes.top().emplace(std::make_pair(node, Node_Key_Null::Create()), variable_indices);
    }

    const std::shared_ptr<Network> network;
    const std::shared_ptr<Concurrency::Job_Queue> job_queue;
    const bool user_command;
    std::stack<std::pair<std::string, std::shared_ptr<const Rete::Symbol>>> symbols;
    std::stack<std::stack<std::pair<std::pair<std::shared_ptr<Zeni::Rete::Node>, std::shared_ptr<const Node_Key>>, std::shared_ptr<Variable_Indices>>>> nodes;
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
  struct Action<Condition_Begin> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cerr << "Condition_Begin: " << input.string() << std::endl;

      data.nodes.emplace(std::stack<std::pair<std::pair<std::shared_ptr<Zeni::Rete::Node>, std::shared_ptr<const Node_Key>>, std::shared_ptr<Variable_Indices>>>());
    }
  };

  template <>
  struct Action<Condition_Attr_Value> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cerr << "Condition_Attr_Value: " << input.string() << std::endl;

      assert(data.symbols.size() == 3);
      const auto third = data.symbols.top();
      data.symbols.pop();
      const auto second = data.symbols.top();
      data.symbols.pop();
      const auto first = data.symbols.top();
      // Deliberately leave the first symbol on the stack

      std::shared_ptr<Zeni::Rete::Node> node = data.network;
      auto variable_indices = Variable_Indices::Create();

      std::shared_ptr<const Node_Key> key;
      if (first.second)
        key = Node_Key_Symbol::Create(first.second);
      else {
        key = Node_Key_Null::Create();
        variable_indices->insert(first.first, Token_Index(0, 0, Symbol_Variable::First));
      }

      if (second.second) {
        node = Node_Filter_1::Create(data.network, data.job_queue, key);
        key = Node_Key_Symbol::Create(second.second);
      }
      else {
        if (!first.second && first.first == second.first) {
          node = Node_Filter_1::Create(data.network, data.job_queue, key);
          key = Node_Key_01::Create();
        }
        else
          variable_indices->insert(second.first, Token_Index(0, 0, Symbol_Variable::Second));
      }

      if (third.second) {
        node = Node_Filter_2::Create(data.network, data.job_queue, key, node);
        key = Node_Key_Symbol::Create(third.second);
      }
      else {
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

      data.nodes.top().emplace(std::make_pair(node, key), variable_indices);

      if (data.nodes.top().size() == 2)
        data.join_conditions<Node_Join>();
    }
  };

  template <>
  struct Action<Condition_End> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cerr << "Condition_End: " << input.string() << std::endl;

      assert(data.symbols.size() == 1);
      data.symbols.pop();

      assert(data.nodes.top().size() == 1);
      const auto node = data.nodes.top().top();
      data.nodes.pop();
      data.nodes.top().emplace(node);
    }
  };

  template <>
  struct Action<Inner_Scope_Begin> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cerr << "Inner_Scope_Begin: " << input.string() << std::endl;

      data.nodes.emplace(std::stack<std::pair<std::pair<std::shared_ptr<Zeni::Rete::Node>, std::shared_ptr<const Node_Key>>, std::shared_ptr<Variable_Indices>>>());
    }
  };

  template <>
  struct Action<Inner_Scope_End> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cerr << "Inner_Scope_End: " << input.string() << std::endl;

      assert(data.nodes.top().size() == 1);
      auto top = data.nodes.top().top();
      data.nodes.pop();
      data.nodes.top().emplace(std::move(top));
    }
  };

  template <>
  struct Action<Scope> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cout << "Scope: " << input.string() << std::endl;

      data.join_conditions<Node_Join>();
    }
  };

  template <>
  struct Action<Scope_Existential> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cout << "Scope_Existential: " << input.string() << std::endl;

      data.join_conditions<Node_Join_Existential>();
    }
  };

  template <>
  struct Action<Scope_Negation> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cout << "Scope_Negation: " << input.string() << std::endl;

      data.join_conditions<Node_Join_Negation>();
    }
  };

  template <>
  struct Action<Source_Production> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cout << "Source_Production: " << input.string() << std::endl;

      const auto node = data.nodes.top().top();
      data.nodes.pop();

      Zeni::Rete::Node_Action::Create(data.network, data.job_queue, data.rule_name, data.user_command, node.first.second, node.first.first, node.second,
        [](const Zeni::Rete::Node_Action &rete_action, const Zeni::Rete::Token &token) {
        std::cout << rete_action.get_name() << " +: " << token << std::endl;
      }, [](const Zeni::Rete::Node_Action &rete_action, const Zeni::Rete::Token &token) {
        std::cout << rete_action.get_name() << " -: " << token << std::endl;
      });
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
