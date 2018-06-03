#ifndef ZENI_RETE_RAVEN_TOKEN_INSERT_HPP
#define ZENI_RETE_RAVEN_TOKEN_INSERT_HPP

#include "Raven_Token.hpp"

namespace Zeni::Rete {

  class Raven_Token_Insert : public Raven_Token {
    Raven_Token_Insert(const Raven_Token_Insert &) = delete;
    Raven_Token_Insert operator=(const Raven_Token_Insert &) = delete;

  public:
    ZENI_RETE_LINKAGE Raven_Token_Insert(const std::shared_ptr<Node> recipient, const std::shared_ptr<Network> network, const std::shared_ptr<const Node> sender, const std::shared_ptr<const Token> token);

    void receive() const override;
  };

}

#endif
