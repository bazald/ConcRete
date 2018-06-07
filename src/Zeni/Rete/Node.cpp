#include "Zeni/Rete/Node.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Internal/Debug_Counters.hpp"
#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Raven_Connect_Gate.hpp"
#include "Zeni/Rete/Raven_Connect_Output.hpp"
#include "Zeni/Rete/Raven_Decrement_Output_Count.hpp"
#include "Zeni/Rete/Raven_Disconnect_Gate.hpp"
#include "Zeni/Rete/Raven_Disconnect_Output.hpp"
#include "Zeni/Rete/Raven_Status_Empty.hpp"
#include "Zeni/Rete/Raven_Status_Nonempty.hpp"
#include "Zeni/Rete/Raven_Token_Insert.hpp"
#include "Zeni/Rete/Raven_Token_Remove.hpp"

#include <cassert>

namespace Zeni::Rete {

  std::shared_ptr<const Node> Node::shared_from_this() const {
    return std::static_pointer_cast<const Node>(Concurrency::Maester::shared_from_this());
  }

  std::shared_ptr<Node> Node::shared_from_this() {
    return std::static_pointer_cast<Node>(Concurrency::Maester::shared_from_this());
  }

  Node::Node(const int64_t height, const int64_t size, const int64_t token_size)
    : m_height(height), m_size(size), m_token_size(token_size),
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
    for (;;) {
      int64_t child_count = m_child_count.load(std::memory_order_acquire);
      if (child_count == 0)
        return false;
      if (m_child_count.compare_exchange_weak(child_count, child_count + 1, std::memory_order_acq_rel))
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
    return m_child_count.fetch_sub(1, std::memory_order_acq_rel) - 1;
#endif
  }

  std::shared_ptr<Node> Node::connect_gate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> output) {
    const auto sft = shared_from_this();

    if (network->get_Node_Sharing() == Network::Node_Sharing::Enabled) {
      Locked_Node_Data locked_node_data(this);

      const Outputs &gates = locked_node_data.get_gates();

      /// TODO: Find a way to kill this loop! Optimize!
      for (auto &existing_output : gates.positive) {
        if (*existing_output == *output) {
          if (existing_output->try_increment_child_count()) {
            DEBUG_COUNTER_DECREMENT(g_node_increments, 1);
            return existing_output;
          }
        }
      }
    }

    job_queue->give_one(std::make_shared<Raven_Connect_Gate>(sft, network, output));

    return output;
  }

  std::shared_ptr<Node> Node::connect_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> output) {
    const auto sft = shared_from_this();

    if (network->get_Node_Sharing() == Network::Node_Sharing::Enabled) {

      Locked_Node_Data locked_node_data(this);

      const Outputs &outputs = locked_node_data.get_outputs();

      /// TODO: Find a way to kill this loop! Optimize!
      for (auto &existing_output : outputs.positive) {
        if (*existing_output == *output) {
          if (existing_output->try_increment_child_count()) {
            DEBUG_COUNTER_DECREMENT(g_node_increments, 1);
            return existing_output;
          }
        }
      }
    }

    job_queue->give_one(std::make_shared<Raven_Connect_Output>(sft, network, output));

    return output;
  }

  void Node::receive(const std::shared_ptr<const Concurrency::Raven> raven) noexcept {
    std::dynamic_pointer_cast<const Rete::Raven>(raven)->receive();
  }

  void Node::receive(const Raven_Connect_Gate &raven) {
    Locked_Node_Data locked_node_data(this);
    receive(raven, locked_node_data);
  }

  void Node::receive(const Raven_Connect_Output &raven) {
    Locked_Node_Data locked_node_data(this);
    receive(raven, locked_node_data);
  }

  void Node::receive(const Raven_Decrement_Output_Count &raven) {
      if (decrement_child_count() == 0)
        send_disconnect_from_parents(raven.get_Network(), raven.get_Job_Queue());
  }

  void Node::receive(const Raven_Disconnect_Gate &raven) {
    Locked_Node_Data locked_node_data(this);
    receive(raven, locked_node_data);
  }

  void Node::receive(const Raven_Disconnect_Output &raven) {
    Locked_Node_Data locked_node_data(this);
    receive(raven, locked_node_data);
  }

  bool Node::receive(const Raven_Connect_Gate &raven, Locked_Node_Data &locked_node_data) {
    const auto sft = shared_from_this();
    std::shared_ptr<Concurrency::IJob> job;
    bool first_insertion = false;

    {
      Outputs &gates = locked_node_data.modify_gates();

      const auto found = gates.negative.equal_range(std::const_pointer_cast<Node>(raven.get_sender()));
      if (found.first != found.second) {
        gates.negative.erase(found.first);
        return false;
      }

      first_insertion = gates.positive.find(std::const_pointer_cast<Node>(raven.get_sender())) == gates.positive.cend();
      gates.positive.emplace(std::const_pointer_cast<Node>(raven.get_sender()));

      if (first_insertion && !locked_node_data.get_output_tokens().empty())
        job = std::make_shared<Raven_Status_Nonempty>(std::const_pointer_cast<Node>(raven.get_sender()), raven.get_Network(), sft);
    }

    if (job)
      raven.get_Job_Queue()->give_one(job);

    return first_insertion;
  }

  bool Node::receive(const Raven_Connect_Output &raven, Locked_Node_Data &locked_node_data) {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;
    bool first_insertion = false;

    {
      Outputs &outputs = locked_node_data.modify_outputs();

      const auto found = outputs.negative.equal_range(std::const_pointer_cast<Node>(raven.get_sender()));
      if (found.first != found.second) {
        outputs.negative.erase(found.first);
        return false;
      }

      first_insertion = outputs.positive.find(std::const_pointer_cast<Node>(raven.get_sender())) == outputs.positive.cend();
      outputs.positive.emplace(std::const_pointer_cast<Node>(raven.get_sender()));

      if (first_insertion) {
        jobs.reserve(locked_node_data.get_output_tokens().size());
        for (auto &output_token : locked_node_data.get_output_tokens())
          jobs.emplace_back(std::make_shared<Raven_Token_Insert>(std::const_pointer_cast<Node>(raven.get_sender()), raven.get_Network(), sft, output_token));
      }
    }

    raven.get_Job_Queue()->give_many(std::move(jobs));

    return first_insertion;
  }

  bool Node::receive(const Raven_Disconnect_Gate &raven, Locked_Node_Data &locked_node_data) {
    const auto sft = shared_from_this();
    std::shared_ptr<Concurrency::IJob> job;
    bool erased_last = false;

    {
      Outputs &gates = locked_node_data.modify_gates();

      auto found = gates.positive.equal_range(std::const_pointer_cast<Node>(raven.get_sender()));
      if (found.first != found.second) {
        gates.positive.erase(found.first++);
        erased_last = found.first == found.second;
      }
      else
        gates.negative.emplace(std::const_pointer_cast<Node>(raven.get_sender()));

      if (raven.decrement_output_count && decrement_child_count() == 0)
        send_disconnect_from_parents(raven.get_Network(), raven.get_Job_Queue());

      if (erased_last && !locked_node_data.get_output_tokens().empty())
        job = std::make_shared<Raven_Status_Empty>(std::const_pointer_cast<Node>(raven.get_sender()), raven.get_Network(), sft);
    }

    if (job)
      raven.get_Job_Queue()->give_one(job);

    return erased_last;
  }

  bool Node::receive(const Raven_Disconnect_Output &raven, Locked_Node_Data &locked_node_data) {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;
    bool erased_last = false;

    {
      Outputs &outputs = locked_node_data.modify_outputs();

      auto found = outputs.positive.equal_range(std::const_pointer_cast<Node>(raven.get_sender()));
      if (found.first != found.second) {
        outputs.positive.erase(found.first++);
        erased_last = found.first == found.second;
      }
      else
        outputs.negative.emplace(std::const_pointer_cast<Node>(raven.get_sender()));

      if (raven.decrement_output_count && decrement_child_count() == 0)
        send_disconnect_from_parents(raven.get_Network(), raven.get_Job_Queue());

      if (erased_last) {
        jobs.reserve(locked_node_data.get_output_tokens().size());
        for (auto &output_token : locked_node_data.get_output_tokens())
          jobs.emplace_back(std::make_shared<Raven_Token_Remove>(std::const_pointer_cast<Node>(raven.get_sender()), raven.get_Network(), sft, output_token));
      }
    }

    raven.get_Job_Queue()->give_many(std::move(jobs));

    return erased_last;
  }

}
