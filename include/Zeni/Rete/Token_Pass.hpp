#ifndef ZENI_RETE_TOKEN_PASS_H
#define ZENI_RETE_TOKEN_PASS_H

#include "Zeni/Concurrency/Raven.hpp"
#include "Zeni/Rete/Node.hpp"

namespace Zeni {

  namespace Rete {

    class Network;
    class Token;

    class Token_Pass : public Concurrency::Raven {
      Token_Pass(const Token_Pass &) = delete;
      Token_Pass operator=(const Token_Pass &) = delete;

    public:
      enum class Type {Action, Retraction};

      ZENI_RETE_LINKAGE Token_Pass(const std::shared_ptr<Node> &recipient, const std::shared_ptr<Network> &network, const std::shared_ptr<const Node> &sender, const std::shared_ptr<const Token> &token, const Type &type)
        : Raven(recipient), m_network(network), m_sender(sender), m_token(token), m_type(type)
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

      ZENI_RETE_LINKAGE Type get_Type() const {
        return m_type;
      }

    private:
      const std::shared_ptr<Network> m_network;
      const std::shared_ptr<const Node> m_sender;
      const std::shared_ptr<const Token> m_token;
      const Type m_type;
    };


  }

}

#endif
