#include "Zeni/Rete/Parser.hpp"

#include "Zeni/Rete/Parser/Impl.hpp"

namespace Zeni::Rete {

  std::shared_ptr<Parser> Parser::Create() {
    return Parser_Impl::Create();
  }

}
