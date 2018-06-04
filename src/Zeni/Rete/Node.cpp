#include "Zeni/Rete/Node.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
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

  Node::Node(const int64_t height, const int64_t size, const int64_t token_size, const bool increment_output_count)
    : m_height(height), m_size(size), m_token_size(token_size),
    m_unlocked_node_data(std::make_shared<Unlocked_Node_Data>(increment_output_count))
  {
  }

  Node::Unlocked_Node_Data::Unlocked_Node_Data(const bool increment_output_count)
    : m_output_count(increment_output_count ? 1 : 0)
  {
  }

  Node::Locked_Node_Data_Const::Locked_Node_Data_Const(const Node * node)
    : m_lock(node->m_mutex),
    m_data(node->m_unlocked_node_data)
  {
  }

  int64_t Node::Locked_Node_Data_Const::get_output_count() const {
    return m_data->m_output_count;
  }

  const Node::Outputs & Node::Locked_Node_Data_Const::get_outputs() const {
    return m_data->m_outputs;
  }

  const Node::Outputs & Node::Locked_Node_Data_Const::get_antioutputs() const {
    return m_data->m_antioutputs;
  }

  const Tokens & Node::Locked_Node_Data_Const::get_output_tokens() const {
    return m_data->m_output_tokens;
  }

  const Node::Outputs & Node::Locked_Node_Data_Const::get_gates() const {
    return m_data->m_gates;
  }

  const Node::Outputs & Node::Locked_Node_Data_Const::get_antigates() const {
    return m_data->m_antigates;
  }

  Node::Locked_Node_Data::Locked_Node_Data(Node * node)
    : Locked_Node_Data_Const(node),
    m_data(node->m_unlocked_node_data)
  {
  }

  int64_t & Node::Locked_Node_Data::modify_output_count() {
    return m_data->m_output_count;
  }

  Node::Outputs & Node::Locked_Node_Data::modify_outputs() {
    return m_data->m_outputs;
  }

  Node::Outputs & Node::Locked_Node_Data::modify_antioutputs() {
    return m_data->m_antioutputs;
  }

  Tokens & Node::Locked_Node_Data::modify_output_tokens() {
    return m_data->m_output_tokens;
  }

  Node::Outputs & Node::Locked_Node_Data::modify_gates() {
    return m_data->m_gates;
  }

  Node::Outputs & Node::Locked_Node_Data::modify_antigates() {
    return m_data->m_antigates;
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

  void Node::increment_output_count() {
    Locked_Node_Data locked_node_data(this);
    ++locked_node_data.modify_output_count();
  }

  std::shared_ptr<Node> Node::connect_gate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> output, const bool immediate) {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::Job>> jobs;

    {
      Locked_Node_Data locked_node_data(this);

      if (network->get_Node_Sharing() == Network::Node_Sharing::Enabled) {
        for (auto &existing_output : locked_node_data.get_gates()) {
          if (*existing_output == *output) {
            if (immediate)
              existing_output->increment_output_count();
            return existing_output;
          }
        }
      }
    }

    if (immediate)
      job_queue->give_one(std::make_shared<Raven_Connect_Gate>(sft, network, output));

    return output;
  }

  std::shared_ptr<Node> Node::connect_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> output, const bool immediate) {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::Job>> jobs;

    {
      Locked_Node_Data locked_node_data(this);

      if (network->get_Node_Sharing() == Network::Node_Sharing::Enabled) {
        for (auto &existing_output : locked_node_data.get_outputs()) {
          if (*existing_output == *output) {
            if (immediate)
              existing_output->increment_output_count();
            return existing_output;
          }
        }
      }
    }

    if (immediate)
      job_queue->give_one(std::make_shared<Raven_Connect_Output>(sft, network, output));

    return output;
  }

  void Node::receive(const std::shared_ptr<const Concurrency::Raven> raven) noexcept {
    std::dynamic_pointer_cast<const Rete::Raven>(raven)->receive();
  }

  void Node::receive(const Raven_Connect_Gate &raven) {
    const auto sft = shared_from_this();
    std::shared_ptr<Concurrency::Job> job;

    {
      Locked_Node_Data locked_node_data(this);

      const auto found = locked_node_data.get_antigates().equal_range(std::const_pointer_cast<Node>(raven.get_sender()));
      if (found.first != found.second) {
        locked_node_data.modify_antigates().erase(found.first);
        return;
      }

      //const auto found2 = locked_node_data.get_gates().find(std::const_pointer_cast<Node>(raven.get_sender()));

      locked_node_data.modify_gates().emplace(std::const_pointer_cast<Node>(raven.get_sender()));

      //if (found2 == locked_node_data.get_gates().end())
      {
        if (!locked_node_data.get_output_tokens().empty())
          job = std::make_shared<Raven_Status_Nonempty>(std::const_pointer_cast<Node>(raven.get_sender()), raven.get_Network(), sft);
      }
    }

    if (job)
      raven.get_Job_Queue()->give_one(job);
  }

  void Node::receive(const Raven_Connect_Output &raven) {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::Job>> jobs;

    {
      Locked_Node_Data locked_node_data(this);

      const auto found = locked_node_data.get_antioutputs().equal_range(std::const_pointer_cast<Node>(raven.get_sender()));
      if (found.first != found.second) {
        locked_node_data.modify_antioutputs().erase(found.first);
        return;
      }

      //const auto found2 = locked_node_data.get_outputs().find(std::const_pointer_cast<Node>(raven.get_sender()));

      locked_node_data.modify_outputs().emplace(std::const_pointer_cast<Node>(raven.get_sender()));

      //if (found2 == locked_node_data.get_outputs().end())
      {
        jobs.reserve(locked_node_data.get_output_tokens().size());
        for (auto &output_token : locked_node_data.get_output_tokens())
          jobs.emplace_back(std::make_shared<Raven_Token_Insert>(std::const_pointer_cast<Node>(raven.get_sender()), raven.get_Network(), sft, output_token));
      }
    }

    raven.get_Job_Queue()->give_many(std::move(jobs));
  }

  void Node::receive(const Raven_Decrement_Output_Count &raven) {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::Job>> jobs;

    {
      Locked_Node_Data locked_node_data(this);

        --locked_node_data.modify_output_count();
        if (locked_node_data.get_output_count() == 0)
          send_disconnect_from_parents(raven.get_Network(), raven.get_Job_Queue(), locked_node_data);
    }
  }

  bool Node::receive(const Raven_Disconnect_Gate &raven) {
    const auto sft = shared_from_this();
    std::shared_ptr<Concurrency::Job> job;
    bool erased_last = false;

    {
      Locked_Node_Data locked_node_data(this);

      auto found = locked_node_data.get_gates().equal_range(std::const_pointer_cast<Node>(raven.get_sender()));
      if (found.first != found.second) {
        locked_node_data.modify_gates().erase(found.first++);
        erased_last = found.first == found.second;
      }
      else
        locked_node_data.modify_antigates().emplace(std::const_pointer_cast<Node>(raven.get_sender()));

      if (raven.decrement_output_count) {
        --locked_node_data.modify_output_count();
        if (locked_node_data.get_output_count() == 0)
          send_disconnect_from_parents(raven.get_Network(), raven.get_Job_Queue(), locked_node_data);
      }

      if (erased_last && !locked_node_data.get_output_tokens().empty())
        job = std::make_shared<Raven_Status_Empty>(std::const_pointer_cast<Node>(raven.get_sender()), raven.get_Network(), sft);
    }

    if (job)
      raven.get_Job_Queue()->give_one(job);

    return erased_last;
  }

  bool Node::receive(const Raven_Disconnect_Output &raven) {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::Job>> jobs;
    bool erased_last = false;

    {
      Locked_Node_Data locked_node_data(this);

      auto found = locked_node_data.get_outputs().equal_range(std::const_pointer_cast<Node>(raven.get_sender()));
      if (found.first != found.second) {
        locked_node_data.modify_outputs().erase(found.first++);
        erased_last = found.first == found.second;
      }
      else
        locked_node_data.modify_antioutputs().emplace(std::const_pointer_cast<Node>(raven.get_sender()));

      if (raven.decrement_output_count) {
        --locked_node_data.modify_output_count();
        if (locked_node_data.get_output_count() == 0)
          send_disconnect_from_parents(raven.get_Network(), raven.get_Job_Queue(), locked_node_data);
      }

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
