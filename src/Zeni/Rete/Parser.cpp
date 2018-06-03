#include "Zeni/Rete/Parser.hpp"

#include <cstdio>
#include <iostream>
#include <stack>
#include <string_view>

#define TAO_PEGTL_NAMESPACE Zeni_Rete_PEG

#include "tao/pegtl.hpp"
#include "tao/pegtl/analyze.hpp"

#include "Zeni/Concurrency/Thread_Pool.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Node_Filter.hpp"
#include "Zeni/Rete/Symbol.hpp"

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
    std::stack<std::pair<std::string, std::shared_ptr<Rete::Symbol>>> symbols;
    std::stack<std::shared_ptr<Node_Filter>> filters;
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

      //std::cout << "Condition: " << input.string() << std::endl;

      data.filters.emplace(Node_Filter::Create(data.network, data.job_queue, WME(first.second, second.second, third.second)));
    }
  };

  //template <>
  //struct Action<Conditions> {
  //  template<typename Input>
  //  static void apply(const Input &input, Data &data) {
  //    std::cout << "Conditions: " << input.string() << std::endl;
  //  }
  //};

  template <>
  struct Action<Source_Production> {
    template<typename Input>
    static void apply(const Input &, Data &data) {
      //std::cout << "Source_Production: " << input.string() << std::endl;

      while (!data.filters.empty()) {
        Zeni::Rete::Node_Action::Create(data.network, data.job_queue, data.rule_name, data.user_command, data.filters.top(), std::make_shared<Zeni::Rete::Variable_Indices>(),
          [](const Zeni::Rete::Node_Action &rete_action, const Zeni::Rete::Token &token) {
          std::cout << rete_action.get_name() << " +: " << token << std::endl;
        }, [](const Zeni::Rete::Node_Action &rete_action, const Zeni::Rete::Token &token) {
          std::cout << rete_action.get_name() << " -: " << token << std::endl;
        });
        data.filters.pop();
      }
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

  class Parser_Pimpl : public std::enable_shared_from_this<Parser_Pimpl> {
    template <typename Input>
    void parse(Input &input, const std::shared_ptr<Network> network, const bool user_command) {
      PEG::Data data(network, network->get_Thread_Pool()->get_main_Job_Queue(), user_command);

      PEG::parse<PEG::Grammar, PEG::Action>(input, data);
    }

  public:
    Parser_Pimpl() {
      Parser_Analyzer::get();
    }

    void parse_file(const std::shared_ptr<Network> network, const std::string &filename, const bool user_command) {
#ifdef _MSC_VER
      FILE * in_file;
      fopen_s(&in_file, filename.c_str(), "r");
#else
      FILE * in_file = std::fopen(filename.c_str(), "r");
#endif
      PEG::read_input<PEG::tracking_mode::LAZY> input(in_file, filename);

      parse(input, network, user_command);

      std::fclose(in_file);
    }

    void parse_string(const std::shared_ptr<Network> network, const std::string_view str, const bool user_command) {
      PEG::memory_input<PEG::tracking_mode::LAZY>
        input(str.data(), str.data() + str.length(), "Zeni::Parser::parse_string(const std::shared_ptr<Network> &, const std::string_view )");

      parse(input, network, user_command);
    }

  private:
    std::shared_ptr<Parser_Pimpl> m_impl;
  };

  Parser::Parser()
    : m_impl(std::make_shared<Parser_Pimpl>())
  {
  }

  void Parser::parse_file(const std::shared_ptr<Network> network, const std::string &filename, const bool user_command) {
    m_impl->parse_file(network, filename, user_command);
  }

  void Parser::parse_string(const std::shared_ptr<Network> network, const std::string_view str, const bool user_command) {
    m_impl->parse_string(network, str, user_command);
  }

}
