#include "Zeni/Rete/Internal/Parser_Impl.hpp"

namespace Zeni::Rete {

  std::shared_ptr<Parser> Parser::Create() {
    return Parser_Impl::Create();
  }

}
