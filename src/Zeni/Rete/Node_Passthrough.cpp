#include "Zeni/Rete/Node_Passthrough.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Internal/Debug_Counters.hpp"
#include "Zeni/Rete/Message_Decrement_Child_Count.hpp"
#include "Zeni/Rete/Message_Status_Empty.hpp"
#include "Zeni/Rete/Message_Status_Nonempty.hpp"
#include "Zeni/Rete/Message_Token_Insert.hpp"
#include "Zeni/Rete/Message_Token_Remove.hpp"
#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Action.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Passthrough::Node_Passthrough(const std::shared_ptr<Node> input)
    : Node_Unary(input->get_height() + 1, input->get_size() + 1, input->get_token_size(), input)
  {
  }

  Node_Passthrough::~Node_Passthrough()
  {
  }

  std::shared_ptr<Node_Passthrough> Node_Passthrough::Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> input) {
    class Friendly_Node_Passthrough : public Node_Passthrough {
    public:
      Friendly_Node_Passthrough(const std::shared_ptr<Node> &input) : Node_Passthrough(input) {}
    };

    const auto created = std::make_shared<Friendly_Node_Passthrough>(input);
    const auto connected = std::static_pointer_cast<Node_Passthrough>(input->connect_output(network, job_queue, created));

    if (connected != created) {
      DEBUG_COUNTER_DECREMENT(g_decrement_children_received, 1);
      job_queue->give_one(std::make_shared<Message_Decrement_Child_Count>(input, network));
    }

    return connected;
  }

  void Node_Passthrough::receive(const Message_Status_Empty &) {
    abort();
  }

  void Node_Passthrough::receive(const Message_Status_Nonempty &) {
    abort();
  }

  void Node_Passthrough::receive(const Message_Token_Insert &message) {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;
      
    {
      Locked_Node_Data locked_node_data(this);
      Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

      const Outputs &gates = locked_node_data.get_gates();
      const Outputs &outputs = locked_node_data.get_outputs();
      Tokens_Input &tokens_input = locked_node_unary_data.modify_input_tokens();

      auto found = tokens_input.negative.find(message.token);
      if (found != tokens_input.negative.end()) {
        tokens_input.negative.erase(found);
        return;
      }

      const bool first_insertion = locked_node_data.get_output_tokens().empty();

      tokens_input.positive.emplace(message.token);
      locked_node_data.modify_output_tokens().emplace(message.token);

      jobs.reserve(outputs.positive.size() + (first_insertion ? gates.positive.size() : 0));
      for (auto &output : outputs.positive)
        jobs.emplace_back(std::make_shared<Message_Token_Insert>(output, message.network, sft, message.token));
      if (first_insertion) {
        for (auto &output : gates.positive)
          jobs.emplace_back(std::make_shared<Message_Status_Nonempty>(output, message.network, sft));
      }
    }

    message.get_Job_Queue()->give_many(std::move(jobs));
  }

  void Node_Passthrough::receive(const Message_Token_Remove &message) {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    {
      Locked_Node_Data locked_node_data(this);
      Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

      const Outputs &gates = locked_node_data.get_gates();
      const Outputs &outputs = locked_node_data.get_outputs();
      Tokens_Input &tokens_input = locked_node_unary_data.modify_input_tokens();

      auto found = tokens_input.positive.find(message.token);
      if (found == tokens_input.positive.end()) {
        tokens_input.negative.emplace(message.token);
        return;
      }

      tokens_input.positive.erase(found);
      locked_node_data.modify_output_tokens().erase(locked_node_data.get_output_tokens().find(message.token));

      const bool last_removal = locked_node_data.get_output_tokens().empty();

      jobs.reserve(outputs.positive.size() + (last_removal ? gates.positive.size() : 0));
      for (auto &output : outputs.positive)
        jobs.emplace_back(std::make_shared<Message_Token_Remove>(output, message.network, sft, message.token));
      if (last_removal) {
        for (auto &output : gates.positive)
          jobs.emplace_back(std::make_shared<Message_Status_Empty>(output, message.network, sft));
      }
    }

    message.get_Job_Queue()->give_many(std::move(jobs));
  }

  bool Node_Passthrough::operator==(const Node &rhs) const {
    if (auto rhs_passthrough = dynamic_cast<const Node_Passthrough *>(&rhs)) {
      return get_input() == rhs_passthrough->get_input();
    }

    return false;
  }

}
