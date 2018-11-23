#include "Zeni/Rete/Internal/Node_Binary.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Internal/Job_Sequential_Messages.hpp"
#include "Zeni/Rete/Internal/Message_Disconnect_Output.hpp"
#include "Zeni/Rete/Network.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Binary::Node_Binary(const int64_t height, const int64_t size, const int64_t token_size, const size_t hash, const std::shared_ptr<const Node_Key> key_left, const std::shared_ptr<const Node_Key> key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right)
    : Node(height, size, token_size, hash),
    m_key_left(key_left),
    m_key_right(key_right),
    m_input_left(input_left),
    m_input_right(input_right)
  {
  }

  void Node_Binary::connect_to_parents_again(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) {
    const auto sft = shared_from_this();

    m_input_left->connect_existing_output(network, job_queue, m_key_left, sft);
    if (m_input_left != m_input_right || *m_key_left != *m_key_right)
      m_input_right->connect_existing_output(network, job_queue, m_key_right, sft);
  }

  void Node_Binary::send_disconnect_from_parents(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) {
    const auto sft = shared_from_this();

    if (m_input_left != m_input_right || *m_key_left != *m_key_right) {
      job_queue->give_one(std::make_shared<Job_Sequential_Messages>(network, std::make_pair(
        std::make_shared<Message_Disconnect_Output>(m_input_right, network, m_key_right, sft),
        std::make_shared<Message_Disconnect_Output>(m_input_left, network, m_key_left, sft)
      )));
    }
    else
      job_queue->give_one(std::make_shared<Message_Disconnect_Output>(m_input_left, network, m_key_left, sft));
  }

   std::shared_ptr<const Node_Key> Node_Binary::get_key_left() const {
    return m_key_left;
  }

  std::shared_ptr<const Node_Key> Node_Binary::get_key_right() const {
    return m_key_right;
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

  bool Node_Binary::is_linked(const std::shared_ptr<Node> input, const std::shared_ptr<const Node_Key> key) {
    const Concurrency::Intrusive_Shared_Ptr<Link_Data>::Lock link_data = m_link_data.load(std::memory_order_acquire);
    return link_data->link_status == Link_Status::BOTH_LINKED ||
      (link_data->link_status == Link_Status::LEFT_UNLINKED ?
      (input == get_input_right() && *key == *get_key_right()) :
        (input == get_input_left() && *key == *get_key_left()));
  }

}
