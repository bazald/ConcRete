#ifndef ZENI_RETE_NODE_UNARY_H
#define ZENI_RETE_NODE_UNARY_H

#include "Zeni/Rete/Node.hpp"

namespace Zeni {

  namespace Rete {

    class Node_Unary : public Node {
      Node_Unary(const Node_Unary &);
      Node_Unary & operator=(const Node_Unary &);

    protected:
      Node_Unary(const int64_t &height, const int64_t &size, const int64_t &token_size, const std::shared_ptr<Node> &input);

      ZENI_RETE_LINKAGE void receive(const Raven_Token_Insert &raven) override;
      ZENI_RETE_LINKAGE void receive(const Raven_Token_Remove &raven) override;

    public:
      std::shared_ptr<const Node> get_parent() const;
      std::shared_ptr<Node> get_parent();

      const Tokens & get_input_tokens() const;

    private:
      std::weak_ptr<Node> m_input;
      Tokens m_input_tokens;
    };

  }

}

#endif
