#ifndef ZENI_RETE_RAVEN_TOKEN_H
#define ZENI_RETE_RAVEN_TOKEN_H

#include "Zeni/Concurrency/Raven.hpp"
#include "Zeni/Rete/Node.hpp"

namespace Zeni {

  namespace Rete {

    class Network;
    class Token;

    class Raven_Token : public Concurrency::Raven {
      Raven_Token(const Raven_Token &) = delete;
      Raven_Token & operator=(const Raven_Token &) = delete;

    public:
      ZENI_RETE_LINKAGE Raven_Token(const std::shared_ptr<Node> &recipient, const std::shared_ptr<Network> &network, const std::shared_ptr<const Node> &sender, const std::shared_ptr<const Token> &token)
        : Raven(recipient), m_network(network), m_sender(sender), m_token(token)
      {
      }

      ZENI_RETE_LINKAGE std::shared_ptr<Network> get_Network() const {
        return m_network;
      }

      ZENI_RETE_LINKAGE std::shared_ptr<const Node> get_sender() const {
        return m_sender;
      }

      ZENI_RETE_LINKAGE std::shared_ptr<const Token> get_Token() const {
        return m_token;
      }

      virtual void receive() const = 0;

    private:
      const std::shared_ptr<Network> m_network;
      const std::shared_ptr<const Node> m_sender;
      const std::shared_ptr<const Token> m_token;
    };

  }

}

#endif
