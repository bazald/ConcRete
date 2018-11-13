#ifndef ZENI_RETE_PARSER_HPP
#define ZENI_RETE_PARSER_HPP

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Internal/Linkage.hpp"

#include <memory>
#include <string_view>

namespace Zeni::Rete {

  class Network;

  class Parser : public std::enable_shared_from_this<Parser> {
    Parser(const Parser &) = delete;
    Parser & operator=(const Parser &) = delete;

  protected:
    Parser() = default;

  public:
    class Exit {};

    ZENI_RETE_LINKAGE virtual ~Parser() {}

    ZENI_RETE_LINKAGE static std::shared_ptr<Parser> Create();

    ZENI_RETE_LINKAGE virtual void parse_file(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::string &filename, const bool user_command) = 0;
    ZENI_RETE_LINKAGE virtual void parse_string(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::string_view str, const bool user_command) = 0;
  };

}

#endif
