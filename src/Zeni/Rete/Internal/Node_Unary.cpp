#include "Zeni/Rete/Internal/Node_Unary.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Internal/Message_Disconnect_Output.hpp"
#include "Zeni/Rete/Network.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Unary::Node_Unary(const int64_t height, const int64_t size, const int64_t token_size, const size_t hash, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> input)
    : Node(height, size, token_size, hash),
    m_input(input),
    m_key(key)
  {
  }

  void Node_Unary::send_disconnect_from_parents(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) {
    job_queue->give_one(std::make_shared<Message_Disconnect_Output>(m_input, network, m_key, shared_from_this()));
  }

  std::shared_ptr<const Node_Key> Node_Unary::get_key() const {
    return m_key;
  }

  std::shared_ptr<const Node> Node_Unary::get_input() const {
    return m_input;
  }

  std::shared_ptr<Node> Node_Unary::get_input() {
    return m_input;
  }

}
