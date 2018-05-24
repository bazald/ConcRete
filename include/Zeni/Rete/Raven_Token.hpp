#ifndef ZENI_RETE_RAVEN_TOKEN_H
#define ZENI_RETE_RAVEN_TOKEN_H

#include "Node.hpp"
#include "Raven.hpp"

namespace Zeni {

  namespace Rete {

    class Network;
    class Token;

    class Raven_Token : public Rete::Raven {
      Raven_Token(const Raven_Token &) = delete;
      Raven_Token & operator=(const Raven_Token &) = delete;

    public:
      ZENI_RETE_LINKAGE Raven_Token(const std::shared_ptr<Node> &recipient, const std::shared_ptr<Network> &network, const std::shared_ptr<const Node> &sender, const std::shared_ptr<const Token> &token)
        : Raven(recipient, network, sender), m_token(token)
      {
      }

      ZENI_RETE_LINKAGE std::shared_ptr<const Token> get_Token() const {
        return m_token;
      }

    private:
      const std::shared_ptr<const Token> m_token;
    };

  }

}

#endif
