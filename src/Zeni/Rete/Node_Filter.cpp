#include "Zeni/Rete/Node_Filter.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Internal/Debug_Counters.hpp"
#include "Zeni/Rete/Message_Decrement_Child_Count.hpp"
#include "Zeni/Rete/Message_Disconnect_Output.hpp"
#include "Zeni/Rete/Message_Status_Empty.hpp"
#include "Zeni/Rete/Message_Status_Nonempty.hpp"
#include "Zeni/Rete/Message_Token_Insert.hpp"
#include "Zeni/Rete/Message_Token_Remove.hpp"
#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Token_Alpha.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Filter::Node_Filter(const std::shared_ptr<Network> network, const WME wme_)
    : Node_Unary(1, 1, 1, hash_combine(std::hash<int>()(1), wme_.get_hash()), network),
    m_wme(wme_),
    m_variable(std::make_tuple(std::dynamic_pointer_cast<const Symbol_Variable>(std::get<0>(m_wme.get_symbols())),
      std::dynamic_pointer_cast<const Symbol_Variable>(std::get<1>(m_wme.get_symbols())),
      std::dynamic_pointer_cast<const Symbol_Variable>(std::get<2>(m_wme.get_symbols()))))
  {
  }

  Node_Filter::~Node_Filter()
  {
  }

  const WME & Node_Filter::get_wme() const {
    return m_wme;
  }

  std::shared_ptr<Node_Filter> Node_Filter::Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const WME &wme) {
    class Friendly_Node_Filter : public Node_Filter {
    public:
      Friendly_Node_Filter(const std::shared_ptr<Network> &network, const WME &wme_) : Node_Filter(network, wme_) {}
    };

    const auto created = std::make_shared<Friendly_Node_Filter>(network, wme);
    const auto connected = std::static_pointer_cast<Node_Filter>(network->connect_output(network, job_queue, created));

    if (connected != created) {
      DEBUG_COUNTER_DECREMENT(g_decrement_children_received, 1);
      job_queue->give_one(std::make_shared<Message_Decrement_Child_Count>(network, network));
    }

    return connected;
  }

  void Node_Filter::receive(const Message_Status_Empty &) {
    abort();
  }

  void Node_Filter::receive(const Message_Status_Nonempty &) {
    abort();
  }

  void Node_Filter::receive(const Message_Token_Insert &message) {
    const auto token = std::dynamic_pointer_cast<const Token_Alpha>(message.token);
    assert(token);
    const auto &wme = token->get_wme();

    if (!std::get<0>(m_variable) && *std::get<0>(m_wme.get_symbols()) != *std::get<0>(wme->get_symbols()))
      return;
    if (!std::get<1>(m_variable) && *std::get<1>(m_wme.get_symbols()) != *std::get<1>(wme->get_symbols()))
      return;
    if (!std::get<2>(m_variable) && *std::get<2>(m_wme.get_symbols()) != *std::get<2>(wme->get_symbols()))
      return;

    if (std::get<0>(m_variable) && std::get<1>(m_variable) && *std::get<0>(m_variable) == *std::get<1>(m_variable) && *std::get<0>(wme->get_symbols()) != *std::get<1>(wme->get_symbols()))
      return;
    if (std::get<0>(m_variable) && std::get<2>(m_variable) && *std::get<0>(m_variable) == *std::get<2>(m_variable) && *std::get<0>(wme->get_symbols()) != *std::get<2>(wme->get_symbols()))
      return;
    if (std::get<1>(m_variable) && std::get<2>(m_variable) && *std::get<1>(m_variable) == *std::get<2>(m_variable) && *std::get<1>(wme->get_symbols()) != *std::get<2>(wme->get_symbols()))
      return;

    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;
      
    {
      Locked_Node_Data locked_node_data(this);
      Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

      const Outputs &gates = locked_node_data.get_gates();
      const Outputs &outputs = locked_node_data.get_outputs();
      Tokens_Input &tokens_input = locked_node_unary_data.modify_input_tokens();
      Tokens_Output &tokens_output = locked_node_data.modify_output_tokens();

      if (!tokens_input.try_emplace(token))
        return;

      const bool first_insertion = tokens_output.empty();

      locked_node_data.modify_output_tokens().emplace(token);

      jobs.reserve(outputs.size() + (first_insertion ? gates.size() : 0));
      for (auto &output : outputs)
        jobs.emplace_back(std::make_shared<Message_Token_Insert>(output, message.network, sft, token));
      if (first_insertion) {
        for (auto &output : gates)
          jobs.emplace_back(std::make_shared<Message_Status_Nonempty>(output, message.network, sft));
      }
    }

    message.get_Job_Queue()->give_many(std::move(jobs));
  }

  void Node_Filter::receive(const Message_Token_Remove &message) {
    const auto token = std::dynamic_pointer_cast<const Token_Alpha>(message.token);
    assert(token);
    const auto &wme = token->get_wme();

    if (!std::get<0>(m_variable) && *std::get<0>(m_wme.get_symbols()) != *std::get<0>(wme->get_symbols()))
      return;
    if (!std::get<1>(m_variable) && *std::get<1>(m_wme.get_symbols()) != *std::get<1>(wme->get_symbols()))
      return;
    if (!std::get<2>(m_variable) && *std::get<2>(m_wme.get_symbols()) != *std::get<2>(wme->get_symbols()))
      return;

    if (std::get<0>(m_variable) && std::get<1>(m_variable) && *std::get<0>(m_variable) == *std::get<1>(m_variable) && *std::get<0>(wme->get_symbols()) != *std::get<1>(wme->get_symbols()))
      return;
    if (std::get<0>(m_variable) && std::get<2>(m_variable) && *std::get<0>(m_variable) == *std::get<2>(m_variable) && *std::get<0>(wme->get_symbols()) != *std::get<2>(wme->get_symbols()))
      return;
    if (std::get<1>(m_variable) && std::get<2>(m_variable) && *std::get<1>(m_variable) == *std::get<2>(m_variable) && *std::get<1>(wme->get_symbols()) != *std::get<2>(wme->get_symbols()))
      return;

    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    {
      Locked_Node_Data locked_node_data(this);
      Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

      const Outputs &gates = locked_node_data.get_gates();
      const Outputs &outputs = locked_node_data.get_outputs();
      Tokens_Input &tokens_input = locked_node_unary_data.modify_input_tokens();
      Tokens_Output &tokens_output = locked_node_data.modify_output_tokens();

      if (!tokens_input.try_erase(token))
        return;

      tokens_output.erase(token);

      const bool last_removal = tokens_output.empty();

      jobs.reserve(outputs.size() + (last_removal ? gates.size() : 0));
      for (auto &output : outputs)
        jobs.emplace_back(std::make_shared<Message_Token_Remove>(output, message.network, sft, token));
      if (last_removal) {
        for (auto &output : gates)
          jobs.emplace_back(std::make_shared<Message_Status_Empty>(output, message.network, sft));
      }
    }

    message.get_Job_Queue()->give_many(std::move(jobs));
  }

  bool Node_Filter::operator==(const Node &rhs) const {
    if (auto filter = dynamic_cast<const Node_Filter *>(&rhs)) {
      if ((std::get<0>(m_variable) != nullptr) ^ (std::get<0>(filter->m_variable) != nullptr))
        return false;
      if (!std::get<0>(m_variable) && *std::get<0>(m_wme.get_symbols()) != *std::get<0>(filter->m_wme.get_symbols()))
        return false;
      if ((std::get<1>(m_variable) != nullptr) ^ (std::get<1>(filter->m_variable) != nullptr))
        return false;
      if (!std::get<1>(m_variable) && *std::get<1>(m_wme.get_symbols()) != *std::get<1>(filter->m_wme.get_symbols()))
        return false;
      if ((std::get<2>(m_variable) != nullptr) ^ (std::get<2>(filter->m_variable) != nullptr))
        return false;
      if (!std::get<2>(m_variable) && *std::get<2>(m_wme.get_symbols()) != *std::get<2>(filter->m_wme.get_symbols()))
        return false;

      if (std::get<0>(m_variable) && std::get<1>(m_variable) && ((*std::get<0>(m_variable) == *std::get<1>(m_variable)) ^ (*std::get<0>(filter->m_variable) == *std::get<1>(filter->m_variable))))
        return false;
      if (std::get<0>(m_variable) && std::get<2>(m_variable) && ((*std::get<0>(m_variable) == *std::get<2>(m_variable)) ^ (*std::get<0>(filter->m_variable) == *std::get<2>(filter->m_variable))))
        return false;
      if (std::get<1>(m_variable) && std::get<2>(m_variable) && ((*std::get<1>(m_variable) == *std::get<2>(m_variable)) ^ (*std::get<1>(filter->m_variable) == *std::get<2>(filter->m_variable))))
        return false;

      return true;
    }
    return false;
  }

}
