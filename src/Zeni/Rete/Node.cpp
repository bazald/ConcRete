#include "Zeni/Rete/Node.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Internal/Debug_Counters.hpp"
#include "Zeni/Rete/Internal/Message_Connect_Output.hpp"
#include "Zeni/Rete/Internal/Message_Disconnect_Output.hpp"
#include "Zeni/Rete/Internal/Message_Token_Insert.hpp"
#include "Zeni/Rete/Internal/Message_Token_Remove.hpp"
#include "Zeni/Rete/Network.hpp"

#include <cassert>

namespace Zeni::Rete {

  std::shared_ptr<const Node> Node::shared_from_this() const {
    return std::static_pointer_cast<const Node>(Concurrency::Recipient::shared_from_this());
  }

  std::shared_ptr<Node> Node::shared_from_this() {
    return std::static_pointer_cast<Node>(Concurrency::Recipient::shared_from_this());
  }

  Node::Node(const int64_t height, const int64_t size, const int64_t token_size, const size_t hash)
    : m_height(height), m_size(size), m_token_size(token_size),
    m_hash(hash)
  {
  }

  int64_t Node::get_height() const {
    return m_height;
  }

  int64_t Node::get_size() const {
    return m_size;
  }

  int64_t Node::get_token_size() const {
    return m_token_size;
  }

  std::shared_ptr<Node> Node::connect_new_or_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> child) {
    const auto[result, snapshot, value] = m_node_data.insert<NODE_DATA_SUBTRIE_OUTPUTS>(child);

    if (result == Node_Trie::Result::First_Insertion)
      job_queue->give_one(std::make_shared<Message_Connect_Output>(shared_from_this(), network, std::make_shared<Node::Node_Data_Snapshot>(snapshot), value));
    else
      DEBUG_COUNTER_DECREMENT(g_node_increments, 1);

    return value;
  }

  void Node::connect_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> child) {
    const auto[result, snapshot, value] = m_node_data.insert<NODE_DATA_SUBTRIE_OUTPUTS>(child);

    assert(value == child);

    if (result == Node_Trie::Result::First_Insertion)
      job_queue->give_one(std::make_shared<Message_Connect_Output>(shared_from_this(), network, std::make_shared<Node::Node_Data_Snapshot>(snapshot), value));
  }

  void Node::receive(const std::shared_ptr<const Concurrency::Message> message) noexcept {
    std::dynamic_pointer_cast<const Rete::Message>(message)->receive();
  }

  bool Node::receive(const Message_Connect_Output &message) {
    if (message.snapshot->empty<NODE_DATA_SUBTRIE_TOKEN_OUTPUTS>())
      return false;

    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    for (auto &output_token : message.snapshot->snapshot<NODE_DATA_SUBTRIE_TOKEN_OUTPUTS>())
      jobs.emplace_back(std::make_shared<Message_Token_Insert>(message.child, message.network, sft, output_token));

    message.get_Job_Queue()->give_many(std::move(jobs));

    return true;
  }

  bool Node::receive(const Message_Disconnect_Output &message) {
    const auto[result, snapshot, value] = m_node_data.erase<NODE_DATA_SUBTRIE_OUTPUTS>(message.child);
    if (result != Node_Trie::Result::Last_Removal)
      return false;

    send_disconnect_from_parents(message.network, message.get_Job_Queue());

    const auto sft = shared_from_this();
    const auto output_tokens = snapshot.snapshot<NODE_DATA_SUBTRIE_TOKEN_OUTPUTS>();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    for (auto &output_token : snapshot.snapshot<NODE_DATA_SUBTRIE_TOKEN_OUTPUTS>())
      jobs.emplace_back(std::make_shared<Message_Token_Remove>(message.child, message.network, sft, output_token));

    message.get_Job_Queue()->give_many(std::move(jobs));

    return true;
  }

  size_t Node::get_hash() const {
    return m_hash;
  }

}
