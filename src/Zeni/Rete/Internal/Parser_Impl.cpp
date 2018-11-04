#include "Zeni/Rete/Internal/Parser_Impl.hpp"

namespace Zeni::Rete {

  Parser_Impl::Parser_Impl() {
    Parser_Analyzer::get();
  }

  std::shared_ptr<Parser_Impl> Parser_Impl::Create() {
    class Friendly_Parser_Impl : public Parser_Impl
    {
    };

    return std::make_shared<Friendly_Parser_Impl>();
  }

  void Parser_Impl::parse_file(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::string &filename, const bool user_command) {
#ifdef _MSC_VER
    FILE * in_file;
    fopen_s(&in_file, filename.c_str(), "r");
#else
    FILE * in_file = std::fopen(filename.c_str(), "r");
#endif
    PEG::read_input<PEG::tracking_mode::lazy> input(in_file, filename);

    parse(input, network, job_queue, user_command);

    std::fclose(in_file);
  }

  void Parser_Impl::parse_string(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::string_view str, const bool user_command) {
    PEG::memory_input<PEG::tracking_mode::lazy> input(str.data(), str.data() + str.length(), "Zeni::Parser::parse_string(const std::shared_ptr<Network> &, const std::string_view )");

    parse(input, network, job_queue, user_command);
  }

}
