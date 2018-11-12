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

  struct Data {
    Data(const std::shared_ptr<Network> network_, const std::shared_ptr<Concurrency::Job_Queue> job_queue_, const bool user_command_)
      : network(network_), job_queue(job_queue_), user_command(user_command_)
    {
      nodes.push(std::stack<std::pair<std::pair<std::shared_ptr<Zeni::Rete::Node>, std::shared_ptr<const Node_Key>>, std::shared_ptr<Variable_Indices>>>());
    }

    ~Data() {
      while (!nodes.empty()) {
        while (!nodes.top().empty()) {
          nodes.top().top().first.first->send_disconnect_from_parents(network, job_queue);
          nodes.top().pop();
        }
        nodes.pop();
      }
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

  template<typename Rule>
  struct Error : public normal<Rule>
  {
    static const std::string error_message;

    template <typename Input>
    static void raise(const Input& in, Data &) {
      throw parse_error(error_message, in);
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
  template<> const std::string Error<until<eolf>>::error_message = "Parser error: failed to process comment"; ///< Probably cannot be triggered

  struct space_comment : sor<space, comment> {};

  struct Unquoted_String : seq<plus<alpha>, star<sor<alnum, one<'-', '_', '*'>>>> {};

  struct Constant_Float : if_then_else<plus_minus, must<sor<decimal, inf, nan>>, sor<decimal, inf, nan>> {};
  template<> const std::string Error<sor<decimal, inf, nan>>::error_message = "Parser error: '+'/'-' must be immediately followed by a numerical value to make a valid symbol";
  struct Constant_Int : if_then_else<plus_minus, must<plus<digit>>, plus<digit>> {};
  template<> const std::string Error<plus<digit>>::error_message = "Parser error: '+'/'-' must be immediately followed by a numerical value to make a valid symbol";
  struct Constant_String : Unquoted_String {};
  struct Quoted_Constant_String : if_must<one<'|'>, seq<plus<not_one<'|'>>, one<'|'>>> {};
  template<> const std::string Error<seq<plus<not_one<'|'>>, one<'|'>>>::error_message = "Parser error: quoted string constant not closed (mismatched '|'s?)";
  struct Variable : if_must<one<'<'>, seq<Unquoted_String, one<'>'>>> {};
  template<> const std::string Error<seq<Unquoted_String, one<'>'>>>::error_message = "Parser error: variable not closed (mismatched '<>'s?)";
  struct Unnamed_Variable : seq<one<'{'>, star<space_comment>, one<'}'>> {};

  struct Symbol : sor<seq<at<minus<Constant_Float, Constant_Int>>, Constant_Float>, Constant_Int, Constant_String, Quoted_Constant_String, Variable, Unnamed_Variable> {};

  struct Condition_Attr_Value : seq<star<space_comment>, one<'^'>, star<space_comment>, Symbol, plus<space_comment>, Symbol> {};
  struct Inner_Condition : seq<star<space_comment>, Symbol, star<space_comment>, plus<Condition_Attr_Value>> {};
  struct Condition_Body : seq<star<space_comment>, Inner_Condition, star<space_comment>, one<')'>, star<space_comment>> {};
  struct Condition_End : if_must<one<'('>, Condition_Body> {};
  template<> const std::string Error<Condition_Body>::error_message = "Parser error: invalid condition syntax (mismatched '()'s?)";
  struct Condition_Begin : at<Condition_End> {};
  struct Condition : seq<Condition_Begin, Condition_End> {};

  struct Subnode_First;
  struct Subnode_Rest;

  struct Inner_Scope_Body : seq<Subnode_First, star<space_comment>, star<Subnode_Rest, star<space_comment>>> {};
  struct Inner_Scope_End : must<Inner_Scope_Body> {};
  template<> const std::string Error<Inner_Scope_Body>::error_message = "Parser error: invalid left-hand side for production rule";
  struct Inner_Scope_Begin : at<Inner_Scope_End> {};
  struct Inner_Scope : seq<Inner_Scope_Begin, Inner_Scope_End> {};
  struct Outer_Scope_Body : seq<Inner_Scope, one<'}'>, star<space_comment>> {};
  struct Outer_Scope : if_must<one<'{'>, Outer_Scope_Body> {};
  template<> const std::string Error<Outer_Scope_Body>::error_message = "Parser error: invalid scope syntax (mismatched '{}'s?)";
  struct Scope : seq<sor<Outer_Scope, Condition>, star<space_comment>> {};

  struct Join : Scope {};
  struct EScope : Scope {};
  struct NScope : Scope {};
  struct Join_Existential : if_must<one<'+'>, EScope> {};
  template<> const std::string Error<EScope>::error_message = "Parser error: '+' must be immediately followed by a valid condition or scope";
  struct Join_Negation : if_must<seq<not_at<string<'-', '-', '>'>>, one<'-'>>, NScope> {};
  template<> const std::string Error<NScope>::error_message = "Parser error: '-' must be immediately followed by a valid condition or scope";

  struct Subnode_First : Scope {};
  struct Subnode_Rest : sor<Join, Join_Existential, Join_Negation> {};

  struct Rule_Name : seq<plus<alpha>, star<sor<alnum, one<'-', '_', '*'>>>> {};
  struct Source_Production_Body : seq<star<space_comment>,
    one<'{'>, star<space_comment>,
    Rule_Name, star<space_comment>,
    Inner_Scope, star<space_comment>,
    string<'-', '-', '>'>, star<space_comment>,
    one<'}'>> {};

  struct Source_Production : if_must<seq<opt<one<'s'>>, one<'p'>>, Source_Production_Body> {};
  template<> const std::string Error<Source_Production_Body>::error_message = "Parser error: 'sp' must be followed by a valid production rule body";

  struct Command : sor<Source_Production> {};

  struct ConcRete_Grammar : seq<star<space_comment>, list_tail<Command, star<space_comment>>, eof> {};

  struct ConcRete : must<ConcRete_Grammar> {};
  template<> const std::string Error<ConcRete_Grammar>::error_message = "Parser error: could not process ConcRete grammar";

  template <typename Rule>
  struct Action : nothing<Rule> {};

  template <>
  struct Action<Constant_Float> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      const double d = std::stod(input.string());
      //std::cerr << "Constant_Float: " << d << std::endl;
      data.symbols.emplace(input.string(), std::make_shared<Symbol_Constant_Float>(d));
    }
  };

  template <>
  struct Action<Constant_Int> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      const int64_t i = std::stoll(input.string());
      //std::cerr << "Constant_Int: " << i << std::endl;
      data.symbols.emplace(input.string(), std::make_shared<Symbol_Constant_Int>(i));
    }
  };

  template <>
  struct Action<Constant_String> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cerr << "Constant_String: " << input.string() << std::endl;
      data.symbols.emplace(input.string(), std::make_shared<Symbol_Constant_String>(input.string()));
    }
  };

  template <>
  struct Action<Quoted_Constant_String> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cerr << "Quoted_Constant_String: " << input.string() << std::endl;
      assert(input.string().size() > 2);
      const auto substr = input.string().substr(1, input.string().size() - 2);
      data.symbols.emplace(substr, std::make_shared<Symbol_Constant_String>(substr));
    }
  };

  template <>
  struct Action<Variable> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cerr << "Constant_Variable: " << input.string() << std::endl;
      assert(input.string().size() > 2);
      const auto substr = input.string().substr(1, input.string().size() - 2);
      data.symbols.emplace(substr, nullptr);
    }
  };

  template <>
  struct Action<Unnamed_Variable> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cerr << "Unnamed_Variable: " << input.string() << std::endl;
      data.symbols.emplace("", nullptr);
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
        if (!first.first.empty())
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
        else if (!second.first.empty())
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
        else if (!third.first.empty())
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
  struct Action<Source_Production> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cerr << "Source_Production: " << input.string() << std::endl;

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
