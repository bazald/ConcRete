#ifndef ZENI_RETE_PARSER_H
#define ZENI_RETE_PARSER_H

#include "Network.hpp"

namespace Zeni {

  namespace Rete {

    class Parser_Pimpl;

    class Parser {
    public:
      ZENI_RETE_LINKAGE Parser();

      ZENI_RETE_LINKAGE void parse_rule(const std::shared_ptr<Network> &network, const std::string &rule);

    private:
      std::shared_ptr<Parser_Pimpl> m_impl;
    };

  }

}

#endif
