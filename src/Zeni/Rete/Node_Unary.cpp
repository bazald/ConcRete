#include "Zeni/Rete/Node_Unary.hpp"

#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Raven_Disconnect_Output.hpp"
#include "Zeni/Rete/Raven_Token_Insert.hpp"
#include "Zeni/Rete/Raven_Token_Remove.hpp"

#include <cassert>

namespace Zeni {

  namespace Rete {

    Node_Unary::Unlocked_Node_Unary_Data::Unlocked_Node_Unary_Data(const std::shared_ptr<Pseudonode> &input)
      : m_input(input)
    {
    }

    Node_Unary::Locked_Node_Unary_Data_Const::Locked_Node_Unary_Data_Const(const Node_Unary * const &node, const Locked_Node_Data_Const &)
      : m_data(node->m_unlocked_node_unary_data)
    {
    }

    std::shared_ptr<const Pseudonode> Node_Unary::Locked_Node_Unary_Data_Const::get_input() const {
      return m_data->m_input;
    }

    const Tokens & Node_Unary::Locked_Node_Unary_Data_Const::get_input_tokens() const {
      return m_data->m_input_tokens;
    }

    const Tokens & Node_Unary::Locked_Node_Unary_Data_Const::get_input_antitokens() const {
      return m_data->m_input_antitokens;
    }

    Node_Unary::Locked_Node_Unary_Data::Locked_Node_Unary_Data(Node_Unary * const &node, const Locked_Node_Data &data)
      : Locked_Node_Unary_Data_Const(node, data),
      m_data(node->m_unlocked_node_unary_data)
    {
    }

    std::shared_ptr<Pseudonode> & Node_Unary::Locked_Node_Unary_Data::modify_input() {
      return m_data->m_input;
    }

    Tokens & Node_Unary::Locked_Node_Unary_Data::modify_input_tokens() {
      return m_data->m_input_tokens;
    }

    Tokens & Node_Unary::Locked_Node_Unary_Data::modify_input_antitokens() {
      return m_data->m_input_antitokens;
    }

    Node_Unary::Node_Unary(const int64_t &height, const int64_t &size, const int64_t &token_size, const std::shared_ptr<Pseudonode> &input)
      : Node(height, size, token_size),
      m_unlocked_node_unary_data(std::make_shared<Unlocked_Node_Unary_Data>(input))
    {
    }

    void Node_Unary::send_disconnect_from_parents(const std::shared_ptr<Network> &network, class Locked_Node_Data &locked_node_data) {
      Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

      network->get_Job_Queue()->give(std::make_shared<Raven_Disconnect_Output>(locked_node_unary_data.modify_input(), network, shared_from_this()));
      locked_node_unary_data.modify_input().reset();
    }

    bool Node_Unary::receive(const Raven_Token_Insert &raven) {
      Locked_Node_Data locked_node_data(this);
      Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

      const auto found = locked_node_unary_data.get_input_antitokens().find(raven.get_Token());
      if (found == locked_node_unary_data.get_input_antitokens().end()) {
        locked_node_unary_data.modify_input_tokens().insert(raven.get_Token());
        return true;
      }
      else {
        locked_node_unary_data.modify_input_antitokens().erase(found);
        return false;
      }
    }

    bool Node_Unary::receive(const Raven_Token_Remove &raven) {
      Locked_Node_Data locked_node_data(this);
      Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

      const auto found = locked_node_unary_data.get_input_tokens().find(raven.get_Token());
      if (found != locked_node_unary_data.get_input_tokens().end()) {
        locked_node_unary_data.modify_input_tokens().erase(found);
        return true;
      }
      else {
        locked_node_unary_data.modify_input_antitokens().insert(raven.get_Token());
        return false;
      }
    }

    ZENI_RETE_LINKAGE std::shared_ptr<Pseudonode> Node_Unary::get_input() {
      Locked_Node_Data locked_node_data(this);
      Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

      return locked_node_unary_data.modify_input();
    }

  }

}
