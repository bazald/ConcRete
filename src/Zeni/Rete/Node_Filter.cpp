#include "Zeni/Rete/Node_Filter.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Raven_Disconnect_Output.hpp"
#include "Zeni/Rete/Raven_Status_Empty.hpp"
#include "Zeni/Rete/Raven_Status_Nonempty.hpp"
#include "Zeni/Rete/Raven_Token_Insert.hpp"
#include "Zeni/Rete/Raven_Token_Remove.hpp"
#include "Zeni/Rete/Token_Alpha.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Filter::Node_Filter(const std::shared_ptr<Network> network, const WME wme_)
    : Node_Unary(1, 1, 1, network),
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

  std::shared_ptr<Node_Filter> Node_Filter::Create(const std::shared_ptr<Network> network, const WME &wme) {
    class Friendly_Node_Filter : public Node_Filter {
    public:
      Friendly_Node_Filter(const std::shared_ptr<Network> &network, const WME &wme_) : Node_Filter(network, wme_) {}
    };

    return std::static_pointer_cast<Node_Filter>(network->connect_output(network, std::make_shared<Friendly_Node_Filter>(network, wme), true));
  }

  void Node_Filter::receive(const Raven_Status_Empty &) {
    abort();
  }

  void Node_Filter::receive(const Raven_Status_Nonempty &) {
    abort();
  }

  void Node_Filter::receive(const Raven_Token_Insert &raven) {
    const auto token = std::dynamic_pointer_cast<const Token_Alpha>(raven.get_Token());
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
    std::vector<std::shared_ptr<Concurrency::Job>> jobs;
      
    {
      Locked_Node_Data locked_node_data(this);
      Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

      auto found = locked_node_unary_data.get_input_antitokens().find(token);
      if (found != locked_node_unary_data.get_input_antitokens().end()) {
        locked_node_unary_data.modify_input_antitokens().erase(found);
        return;
      }

      const bool empty = locked_node_data.get_output_tokens().empty();

      locked_node_unary_data.modify_input_tokens().emplace(token);
      locked_node_data.modify_output_tokens().emplace(token);

      jobs.reserve(locked_node_data.get_outputs().size() + (empty ? locked_node_data.get_gates().size() : 0));
      for (auto &output : locked_node_data.get_outputs())
        jobs.emplace_back(std::make_shared<Raven_Token_Insert>(output, raven.get_Network(), sft, token));
      if (empty) {
        for (auto &output : locked_node_data.get_gates())
          jobs.emplace_back(std::make_shared<Raven_Status_Nonempty>(output, raven.get_Network(), sft));
      }
    }

    raven.get_Network()->get_Job_Queue()->give_many(std::move(jobs));
  }

  void Node_Filter::receive(const Raven_Token_Remove &raven) {
    const auto token = std::dynamic_pointer_cast<const Token_Alpha>(raven.get_Token());
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
    std::vector<std::shared_ptr<Concurrency::Job>> jobs;

    {
      Locked_Node_Data locked_node_data(this);
      Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

      auto found = locked_node_unary_data.get_input_tokens().find(token);
      if (found == locked_node_unary_data.get_input_tokens().end()) {
        locked_node_unary_data.modify_input_antitokens().emplace(token);
        return;
      }

      locked_node_unary_data.modify_input_tokens().erase(found);
      locked_node_data.modify_output_tokens().erase(locked_node_data.get_output_tokens().find(token));

      const bool empty = locked_node_data.get_output_tokens().empty();

      jobs.reserve(locked_node_data.get_outputs().size() + (empty ? locked_node_data.get_gates().size() : 0));
      for (auto &output : locked_node_data.get_outputs())
        jobs.emplace_back(std::make_shared<Raven_Token_Remove>(output, raven.get_Network(), sft, token));
      if (empty) {
        for (auto &output : locked_node_data.get_gates())
          jobs.emplace_back(std::make_shared<Raven_Status_Empty>(output, raven.get_Network(), sft));
      }
    }

    raven.get_Network()->get_Job_Queue()->give_many(std::move(jobs));
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
