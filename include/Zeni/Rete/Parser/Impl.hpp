#ifndef ZENI_RETE_PARSER_IMPL_HPP
#define ZENI_RETE_PARSER_IMPL_HPP

#include "Actions_Impl.hpp"
#include "Data.hpp"
#include "PEG.hpp"
#include "../Parser.hpp"

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
    void parse(Input &input, const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action) {
      PEG::Data data(network, job_queue, user_action);

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
    Parser_Impl() {
      Parser_Analyzer::get();
    }

  public:
    ZENI_RETE_LINKAGE static std::shared_ptr<Parser_Impl> Create() {
      class Friendly_Parser_Impl : public Parser_Impl
      {
      };

      return std::make_shared<Friendly_Parser_Impl>();
    }

    void parse_file(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::string &filename, const bool user_action) override {
#ifdef _MSC_VER
      FILE * in_file;
      fopen_s(&in_file, filename.c_str(), "r");
#else
      FILE * in_file = std::fopen(filename.c_str(), "r");
#endif
      PEG::read_input<PEG::tracking_mode::lazy> input(in_file, filename);

      parse(input, network, job_queue, user_action);

      std::fclose(in_file);
    }

    void parse_string(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::string_view str, const bool user_action) override {
      PEG::memory_input<PEG::tracking_mode::lazy> input(str.data(), str.data() + str.length(), "Zeni::Parser::parse_string(const std::shared_ptr<Network> &, const std::string_view )");

      parse(input, network, job_queue, user_action);
    }
  };

}

#endif
