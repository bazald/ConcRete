#include "Zeni/Rete/Node_Unary.hpp"

#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Raven_Token_Insert.hpp"
#include "Zeni/Rete/Raven_Token_Remove.hpp"

#include <cassert>

namespace Zeni {

  namespace Rete {

    Node_Unary::Node_Unary(const int64_t &height, const int64_t &size, const int64_t &token_size, const std::shared_ptr<Node> &input)
      : Node(height, size, token_size),
      m_input(input)
    {
    }

    ZENI_RETE_LINKAGE void Node_Unary::receive(const Raven_Token_Insert &raven) {
      Concurrency::Mutex::Lock lock(m_mutex);
      m_input_tokens.insert(raven.get_Token());
    }

    ZENI_RETE_LINKAGE void Node_Unary::receive(const Raven_Token_Remove &raven) {
      Concurrency::Mutex::Lock lock(m_mutex);
      const auto found = m_input_tokens.find(raven.get_Token());
      assert(found != m_input_tokens.end());
      m_input_tokens.erase(found);
    }

    std::shared_ptr<const Node> Node_Unary::get_parent() const {
      return m_input.lock();
    }

    std::shared_ptr<Node> Node_Unary::get_parent() {
      return m_input.lock();
    }

    const Node::Tokens & Node_Unary::get_input_tokens() const {
      return m_input_tokens;
    }

  }

}
