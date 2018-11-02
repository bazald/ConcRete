#include "Zeni/Rete/Node_Passthrough.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Internal/Debug_Counters.hpp"
#include "Zeni/Rete/Internal/Message_Status_Empty.hpp"
#include "Zeni/Rete/Internal/Message_Status_Nonempty.hpp"
#include "Zeni/Rete/Internal/Message_Token_Insert.hpp"
#include "Zeni/Rete/Internal/Message_Token_Remove.hpp"
#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Action.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Passthrough::Node_Passthrough(const std::shared_ptr<Node> input)
    : Node_Unary(input->get_height() + 1, input->get_size() + 1, input->get_token_size(), hash_combine(std::hash<int>()(2), input->get_hash()), input)
  {
  }

  Node_Passthrough::Node_Passthrough(const size_t hash, const std::shared_ptr<Node> input)
    : Node_Unary(input->get_height() + 1, input->get_size() + 1, input->get_token_size(), hash, input)
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

    const auto created = std::shared_ptr<Friendly_Node_Passthrough>(new Friendly_Node_Passthrough(input));
    const auto connected = std::static_pointer_cast<Node_Passthrough>(input->connect_new_or_existing_output(network, job_queue, created));

    if (connected != created) {
      input->send_disconnect_from_parents(network, job_queue);
      DEBUG_COUNTER_DECREMENT(g_decrement_children_received, 1);
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
    const auto[result, snapshot, value] = m_node_data.insert<NODE_DATA_SUBTRIE_TOKEN_OUTPUTS>(message.token);
    if (result != Output_Token_Trie::Result::First_Insertion)
      return;

    const auto sft = shared_from_this();
    const auto output_tokens = snapshot.template snapshot<NODE_DATA_SUBTRIE_TOKEN_OUTPUTS>();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    for (auto &output : snapshot.template snapshot<NODE_DATA_SUBTRIE_OUTPUTS>())
      jobs.emplace_back(std::make_shared<Message_Token_Insert>(output, message.network, sft, message.token));
    if (++output_tokens.cbegin() == output_tokens.cend()) {
      for (auto &output : snapshot.template snapshot<NODE_DATA_SUBTRIE_GATES>())
        jobs.emplace_back(std::make_shared<Message_Status_Nonempty>(output, message.network, sft));
    }

    message.get_Job_Queue()->give_many(std::move(jobs));
  }

  void Node_Passthrough::receive(const Message_Token_Remove &message) {
    const auto[result, snapshot, value] = m_node_data.erase<NODE_DATA_SUBTRIE_TOKEN_OUTPUTS>(message.token);
    if (result != Output_Token_Trie::Result::Last_Removal)
      return;

    const auto sft = shared_from_this();
    const auto output_tokens = snapshot.template snapshot<NODE_DATA_SUBTRIE_TOKEN_OUTPUTS>();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    for (auto &output : snapshot.template snapshot<NODE_DATA_SUBTRIE_OUTPUTS>())
      jobs.emplace_back(std::make_shared<Message_Token_Remove>(output, message.network, sft, message.token));
    if (output_tokens.cbegin() == output_tokens.cend()) {
      for (auto &output : snapshot.template snapshot<NODE_DATA_SUBTRIE_GATES>())
        jobs.emplace_back(std::make_shared<Message_Status_Empty>(output, message.network, sft));
    }

    message.get_Job_Queue()->give_many(std::move(jobs));
  }

  bool Node_Passthrough::operator==(const Node &rhs) const {
    //return this == &rhs;

    if (auto rhs_passthrough = dynamic_cast<const Node_Passthrough *>(&rhs))
      return get_input() == rhs_passthrough->get_input();

    return false;
  }

}
