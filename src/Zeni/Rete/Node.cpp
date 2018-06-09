#include "Zeni/Rete/Node.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Internal/Debug_Counters.hpp"
#include "Zeni/Rete/Internal/Message_Connect_Gate.hpp"
#include "Zeni/Rete/Internal/Message_Connect_Output.hpp"
#include "Zeni/Rete/Internal/Message_Decrement_Child_Count.hpp"
#include "Zeni/Rete/Internal/Message_Disconnect_Gate.hpp"
#include "Zeni/Rete/Internal/Message_Disconnect_Output.hpp"
#include "Zeni/Rete/Internal/Message_Status_Empty.hpp"
#include "Zeni/Rete/Internal/Message_Status_Nonempty.hpp"
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
    m_hash(hash),
    m_unlocked_node_data(std::make_shared<Unlocked_Node_Data>())
  {
  }

  Node::Unlocked_Node_Data::Unlocked_Node_Data() {
    DEBUG_COUNTER_INCREMENT(g_node_increments, 1);
  }

  Node::Locked_Node_Data_Const::Locked_Node_Data_Const(const Node * node)
    : m_lock(node->m_mutex),
    m_data(node->m_unlocked_node_data)
  {
  }

  const Node::Outputs & Node::Locked_Node_Data_Const::get_outputs() const {
    return m_data->m_outputs;
  }

  const Tokens_Output & Node::Locked_Node_Data_Const::get_output_tokens() const {
    return m_data->m_output_tokens;
  }

  const Node::Outputs & Node::Locked_Node_Data_Const::get_gates() const {
    return m_data->m_gates;
  }

  Node::Locked_Node_Data::Locked_Node_Data(Node * node)
    : Locked_Node_Data_Const(node),
    m_data(node->m_unlocked_node_data)
  {
  }

  Node::Outputs & Node::Locked_Node_Data::modify_outputs() {
    return m_data->m_outputs;
  }

  Tokens_Output & Node::Locked_Node_Data::modify_output_tokens() {
    return m_data->m_output_tokens;
  }

  Node::Outputs & Node::Locked_Node_Data::modify_gates() {
    return m_data->m_gates;
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

  bool Node::try_increment_child_count() {
#ifdef DISABLE_MULTITHREADING
    if (m_child_count == 0)
      return false;
    ++m_child_count;
#else
    int64_t child_count = m_child_count.load();
    for (;;) {
      if (child_count == 0)
        return false;
      if (m_child_count.compare_exchange_weak(child_count, child_count + 1))
        break;
    }
#endif
    DEBUG_COUNTER_INCREMENT(g_node_increments, 1);
    DEBUG_COUNTER_INCREMENT(g_try_increment_child_counts, 1);
    return true;
  }

  int64_t Node::decrement_child_count() {
#ifdef DISABLE_MULTITHREADING
    return --m_child_count;
#else
    return m_child_count.fetch_sub(1) - 1;
#endif
  }

  std::shared_ptr<Node> Node::connect_gate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> child) {
    if (network->get_Node_Sharing() == Network::Node_Sharing::Enabled) {
      Locked_Node_Data locked_node_data(this);

      const Outputs &gates = locked_node_data.get_gates();

      const auto found = gates.find(child);
      if (found != gates.cend()) {
        if ((*found)->try_increment_child_count()) {
          DEBUG_COUNTER_DECREMENT(g_node_increments, 1);
          return *found;
        }
      }
    }

    job_queue->give_one(std::make_shared<Message_Connect_Gate>(shared_from_this(), network, child));

    return child;
  }

  std::shared_ptr<Node> Node::connect_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> child) {
    if (network->get_Node_Sharing() == Network::Node_Sharing::Enabled) {

      Locked_Node_Data locked_node_data(this);

      const Outputs &outputs = locked_node_data.get_outputs();

      const auto found = outputs.find(child);
      if (found != outputs.cend()) {
        if ((*found)->try_increment_child_count()) {
          DEBUG_COUNTER_DECREMENT(g_node_increments, 1);
          return *found;
        }
      }
    }

    job_queue->give_one(std::make_shared<Message_Connect_Output>(shared_from_this(), network, child));

    return child;
  }

  void Node::receive(const std::shared_ptr<const Concurrency::Message> message) noexcept {
    std::dynamic_pointer_cast<const Rete::Message>(message)->receive();
  }

  void Node::receive(const Message_Connect_Gate &message) {
    Locked_Node_Data locked_node_data(this);
    receive(message, locked_node_data);
  }

  void Node::receive(const Message_Connect_Output &message) {
    Locked_Node_Data locked_node_data(this);
    receive(message, locked_node_data);
  }

  void Node::receive(const Message_Decrement_Child_Count &message) {
      if (decrement_child_count() == 0)
        send_disconnect_from_parents(message.network, message.get_Job_Queue());
  }

  void Node::receive(const Message_Disconnect_Gate &message) {
    Locked_Node_Data locked_node_data(this);
    receive(message, locked_node_data);
  }

  void Node::receive(const Message_Disconnect_Output &message) {
    Locked_Node_Data locked_node_data(this);
    receive(message, locked_node_data);
  }

  bool Node::receive(const Message_Connect_Gate &message, Locked_Node_Data &locked_node_data) {
    const auto sft = shared_from_this();
    std::shared_ptr<Concurrency::IJob> job;
    bool first_insertion = false;

    {
      Outputs &gates = locked_node_data.modify_gates();

      first_insertion = gates.try_emplace(message.child);

      if (first_insertion && !locked_node_data.get_output_tokens().empty())
        job = std::make_shared<Message_Status_Nonempty>(message.child, message.network, sft);
    }

    if (job)
      message.get_Job_Queue()->give_one(job);

    return first_insertion;
  }

  bool Node::receive(const Message_Connect_Output &message, Locked_Node_Data &locked_node_data) {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;
    bool first_insertion = false;

    {
      Outputs &outputs = locked_node_data.modify_outputs();

      first_insertion = outputs.try_emplace(message.child);

      if (first_insertion) {
        jobs.reserve(locked_node_data.get_output_tokens().size());
        for (auto &output_token : locked_node_data.get_output_tokens())
          jobs.emplace_back(std::make_shared<Message_Token_Insert>(message.child, message.network, sft, output_token));
      }
    }

    message.get_Job_Queue()->give_many(std::move(jobs));

    return first_insertion;
  }

  bool Node::receive(const Message_Disconnect_Gate &message, Locked_Node_Data &locked_node_data) {
    const auto sft = shared_from_this();
    std::shared_ptr<Concurrency::IJob> job;
    bool erased_last = false;

    {
      Outputs &gates = locked_node_data.modify_gates();

      erased_last = gates.try_erase(message.child);

      if (message.decrement_output_count && decrement_child_count() == 0)
        send_disconnect_from_parents(message.network, message.get_Job_Queue());

      if (erased_last && !locked_node_data.get_output_tokens().empty())
        job = std::make_shared<Message_Status_Empty>(message.child, message.network, sft);
    }

    if (job)
      message.get_Job_Queue()->give_one(job);

    return erased_last;
  }

  bool Node::receive(const Message_Disconnect_Output &message, Locked_Node_Data &locked_node_data) {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;
    bool erased_last = false;

    {
      Outputs &outputs = locked_node_data.modify_outputs();

      erased_last = outputs.try_erase(message.child);

      if (message.decrement_output_count && decrement_child_count() == 0)
        send_disconnect_from_parents(message.network, message.get_Job_Queue());

      if (erased_last) {
        jobs.reserve(locked_node_data.get_output_tokens().size());
        for (auto &output_token : locked_node_data.get_output_tokens())
          jobs.emplace_back(std::make_shared<Message_Token_Remove>(message.child, message.network, sft, output_token));
      }
    }

    message.get_Job_Queue()->give_many(std::move(jobs));

    return erased_last;
  }

  size_t Node::get_hash() const {
    return m_hash;
  }

}
