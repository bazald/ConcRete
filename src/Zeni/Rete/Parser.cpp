#include "Zeni/Rete/Parser.hpp"

#include <cstdio>
#include <iostream>
#include <string_view>

#define TAO_PEGTL_NAMESPACE Zeni_Rete_PEGTL

#include "tao/pegtl.hpp"
#include "tao/pegtl/analyze.hpp"

namespace Zeni {

  namespace Rete {

    namespace PEGTL {

      using namespace tao::Zeni_Rete_PEGTL;

      struct plus_minus : opt<one<'+','-'>> {};
      struct dot : one<'.'> {};

      struct inf : seq<istring<'i','n','f'>,opt<istring<'i','n','i','t','y'>>> {};
      struct nan : seq<istring<'n','a','n'>, opt<one<'('>, plus<alnum>, one<')'>>> {};

      template<typename D>
      struct number : if_then_else<dot, plus<D>, seq<plus<D>, opt<dot, star<D>>>> {};

      struct e : one<'e', 'E'> {};
      struct p : one<'p', 'P'> {};
      struct exponent : seq<plus_minus, plus<digit>> {};

      struct decimal : seq<number<digit>, opt<e, exponent>> {};

      struct at_space_eof : at<sor<space, eof>> {};

      struct Constant_Int : seq<plus_minus, plus<digit>, at_space_eof> {};
      struct Constant_Double : seq<plus_minus, sor<decimal, inf, nan>, at_space_eof> {};
      struct Constant_String : seq<plus<alpha>, star<sor<alnum, one<'-', '_', '*'>>>, at_space_eof> {};
      struct Variable : seq<one<'<'>, plus<alpha>, star<sor<alnum, one<'-', '_', '*'>>>, one<'>'>, at_space_eof> {};

      struct Symbol : sor<Constant_Int, Constant_Double, Constant_String, Variable> {};

      struct Grammar : seq<Symbol, star<star<space>, Symbol>> {};

      template <typename Rule>
      struct Action : nothing<Rule> {};

      template <>
      struct Action<Constant_Double> {
        template<typename Input>
        static void apply(const Input &input, const std::shared_ptr<Network> &network) {
          std::stringstream sin(input.string());
          double d;
          sin >> d;
          std::cout << "Double: " << d << std::endl;
        }
      };

      template <>
      struct Action<Constant_Int> {
        template<typename Input>
        static void apply(const Input &input, const std::shared_ptr<Network> &network) {
          std::stringstream sin(input.string());
          int64_t i;
          sin >> i;
          std::cout << "Int: " << i << std::endl;
        }
      };

      template <>
      struct Action<Constant_String> {
        template<typename Input>
        static void apply(const Input &input, const std::shared_ptr<Network> &network) {
          std::stringstream sin(input.string());
          const std::string str = input.string();
          std::cout << "String: " << str << std::endl;
        }
      };

      template <>
      struct Action<Variable> {
        template<typename Input>
        static void apply(const Input &input, const std::shared_ptr<Network> &network) {
          std::stringstream sin(input.string());
          std::string str = input.string();
          str = str.substr(1, str.length() - 2);
          std::cout << "Variable: " << str << std::endl;
        }
      };
    }

    class Parser_Analyzer {
    public:
      Parser_Analyzer() {
        PEGTL::analyze<PEGTL::Grammar>();
      }
    };

    static Parser_Analyzer m_parser_analyzer;

    class Parser_Pimpl : public std::enable_shared_from_this<Parser_Pimpl> {
      template <typename Input>
      void parse(Input &input, const std::shared_ptr<Network> &network) {
        PEGTL::parse<PEGTL::Grammar, PEGTL::Action>(input, network);
      }

    public:
      Parser_Pimpl() {

      }

      void parse_file(const std::shared_ptr<Network> &network, const std::string &filename) {
        FILE * in_file = std::fopen(filename.c_str(), "r");
        PEGTL::file_input<PEGTL::tracking_mode::LAZY> input(in_file, filename);

        parse(input, network);

        std::fclose(in_file);
      }

      void parse_string(const std::shared_ptr<Network> &network, const std::string_view str) {
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

    void Parser::parse_file(const std::shared_ptr<Network> &network, const std::string &filename) {
      m_impl->parse_file(network, filename);
    }

    void Parser::parse_string(const std::shared_ptr<Network> &network, const std::string_view str) {
      m_impl->parse_string(network, str);
    }

  }

}
