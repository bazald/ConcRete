#include "Zeni/Rete/Internal/Node_Binary.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Internal/Message_Connect_Output.hpp"
#include "Zeni/Rete/Internal/Message_Disconnect_Output.hpp"
#include "Zeni/Rete/Internal/Message_Sequential_Pair.hpp"
#include "Zeni/Rete/Network.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Binary::Node_Binary(const int64_t height, const int64_t size, const int64_t token_size, const size_t hash, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right)
    : Node(height, size, token_size, hash),
    m_input_left(input_left),
    m_input_right(input_right)
  {
  }

  void Node_Binary::send_disconnect_from_parents(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) {
    const auto sft = shared_from_this();

    if (m_input_left != m_input_right) {
      job_queue->give_one(std::make_shared<Message_Sequential_Pair>(network, std::make_pair(
        std::make_shared<Message_Disconnect_Output>(m_input_right, network, sft),
        std::make_shared<Message_Disconnect_Output>(m_input_left, network, sft)
      )));
    }
    else
      job_queue->give_one(std::make_shared<Message_Disconnect_Output>(m_input_left, network, sft));
  }

  std::pair<std::shared_ptr<Node>, std::shared_ptr<Node>> Node_Binary::get_inputs() {
    return std::make_pair(m_input_left, m_input_right);
  }

  std::shared_ptr<const Node> Node_Binary::get_input_left() const {
    return m_input_left;
  }

  std::shared_ptr<Node> Node_Binary::get_input_left() {
    return m_input_left;
  }

  std::shared_ptr<const Node> Node_Binary::get_input_right() const {
    return m_input_right;
  }

  std::shared_ptr<Node> Node_Binary::get_input_right() {
    return m_input_right;
  }

}