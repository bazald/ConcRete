#include "Zeni/Rete/Parser.hpp"

#include <cstdio>
#include <iostream>
#include <stack>
#include <string_view>

#define TAO_PEGTL_NAMESPACE Zeni_Rete_PEGTL

#include "tao/pegtl.hpp"
#include "tao/pegtl/analyze.hpp"

#include "Zeni/Rete/Node_Filter.hpp"
#include "Zeni/Rete/Symbol.hpp"

namespace Zeni::Rete::PEGTL {

  using namespace tao::Zeni_Rete_PEGTL;

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
    Conditions,
    string<'-', '-', '>'>, star<space_comment>,
    one<'}'>> {};

  struct Grammar : must<seq<star<space_comment>, list_tail<Source_Production, star<space_comment>>>, eof> {};

  struct Data {
    Data(const std::shared_ptr<Network> network_) : network(network_) {}

    const std::shared_ptr<Network> network;
    std::stack<std::pair<std::string_view, std::shared_ptr<Rete::Symbol>>> symbols;
    std::stack<std::shared_ptr<Node_Filter>> filter_nodes;
  };

  template <typename Rule>
  struct Action : nothing<Rule> {};

  template <>
  struct Action<Constant_Float> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      std::stringstream sin(input.string());
      double d;
      sin >> d;
      //std::cout << "Double: " << d << std::endl;
      data.symbols.push(std::make_pair(input.string(), std::make_shared<Symbol_Constant_Float>(d)));
    }
  };

  template <>
  struct Action<Constant_Int> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      std::stringstream sin(input.string());
      int64_t i;
      sin >> i;
      //std::cout << "Int: " << i << std::endl;
      data.symbols.push(std::make_pair(input.string(), std::make_shared<Symbol_Constant_Int>(i)));
    }
  };

  template <>
  struct Action<Constant_String> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      std::stringstream sin(input.string());
      const std::string s = input.string();
      //std::cout << "String: " << s << std::endl;
      data.symbols.push(std::make_pair(input.string(), std::make_shared<Symbol_Constant_String>(s)));
    }
  };

  template <>
  struct Action<Variable> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      std::stringstream sin(input.string());
      std::string str = input.string();
      str = str.substr(1, str.length() - 2);
      //std::cout << "Variable: " << str << std::endl;
      data.symbols.push(std::pair(input.string(), nullptr));
    }
  };

  template <>
  struct Action<Condition> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      auto third = data.symbols.top();
      data.symbols.pop();
      auto second = data.symbols.top();
      data.symbols.pop();
      auto first = data.symbols.top();
      data.symbols.pop();

      if (!first.second) {
        first.second = std::make_shared<Symbol_Variable>(Symbol_Variable::First);
        if (!second.second && first.first == second.first)
          second.second = first.second;
        if (!third.second && first.first == third.first)
          third.second = first.second;
      }
      if (!second.second) {
        second.second = std::make_shared<Symbol_Variable>(Symbol_Variable::Second);
        if (!third.second && second.first == third.first)
          third.second = second.second;
      }
      if (!third.second)
        second.second = std::make_shared<Symbol_Variable>(Symbol_Variable::Third);

      auto filter = Node_Filter::Create_Or_Increment_Output_Count(data.network, WME(first.second, second.second, third.second));

      //std::cout << "Condition: " << input.string() << std::endl;
    }
  };

  template <>
  struct Action<Conditions> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      //std::cout << "Conditions: " << input.string() << std::endl;
    }
  };

  template <>
  struct Action<Source_Production> {
    template<typename Input>
    static void apply(const Input &input, Data &data) {
      std::cout << "Source_Production: " << input.string() << std::endl;
    }
  };

}

namespace Zeni::Rete {

  class Parser_Analyzer {
  public:
    Parser_Analyzer() {
      PEGTL::analyze<PEGTL::Grammar>();
    }
  };

  static Parser_Analyzer m_parser_analyzer;

  class Parser_Pimpl : public std::enable_shared_from_this<Parser_Pimpl> {
    template <typename Input>
    void parse(Input &input, const std::shared_ptr<Network> network) {
      PEGTL::Data data(network);

      PEGTL::parse<PEGTL::Grammar, PEGTL::Action>(input, data);
    }

  public:
    Parser_Pimpl() {

    }

    void parse_file(const std::shared_ptr<Network> network, const std::string &filename) {
      FILE * in_file = std::fopen(filename.c_str(), "r");
      PEGTL::file_input<PEGTL::tracking_mode::LAZY> input(in_file, filename);

      parse(input, network);

      std::fclose(in_file);
    }

    void parse_string(const std::shared_ptr<Network> network, const std::string_view str) {
      PEGTL::memory_input<PEGTL::tracking_mode::LAZY>
        input(str.data(), str.data() + str.length(), "Zeni::Parser::parse_string(const std::shared_ptr<Network> &, const std::string_view )");

      parse(input, network);
    }

  private:
    std::shared_ptr<Parser_Pimpl> m_impl;
  };

  Parser::Parser()
    : m_impl(std::make_shared<Parser_Pimpl>())
  {
  }

  void Parser::parse_file(const std::shared_ptr<Network> network, const std::string &filename) {
    m_impl->parse_file(network, filename);
  }

  void Parser::parse_string(const std::shared_ptr<Network> network, const std::string_view str) {
    m_impl->parse_string(network, str);
  }

}
