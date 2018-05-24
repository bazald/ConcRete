#include "Zeni/Rete/Node.hpp"

#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Raven_Disconnect_Output.hpp"
#include "Zeni/Rete/Raven_Token_Insert.hpp"
#include "Zeni/Rete/Raven_Token_Remove.hpp"

#include <cassert>

namespace Zeni {

  namespace Rete {

    std::shared_ptr<const Node> Node::shared_from_this() const {
      return std::static_pointer_cast<const Node>(Concurrency::Maester::shared_from_this());
    }

    std::shared_ptr<Node> Node::shared_from_this() {
      return std::static_pointer_cast<Node>(Concurrency::Maester::shared_from_this());
    }

    Node::Node(const int64_t &height, const int64_t &size, const int64_t &token_size)
      : m_height(height), m_size(size), m_token_size(token_size),
      m_unlocked_node_data(std::make_shared<Unlocked_Node_Data>())
    {
    }

    Node::Locked_Node_Data_Const::Locked_Node_Data_Const(const Node * const &node)
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

    const Tokens & Node::Locked_Node_Data_Const::get_output_tokens() const {
      return m_data->m_output_tokens;
    }

    Node::Locked_Node_Data::Locked_Node_Data(Node * const &node)
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

    Tokens & Node::Locked_Node_Data::modify_output_tokens() {
      return m_data->m_output_tokens;
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

    void Node::connect_output(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output) {
      Tokens output_tokens;

      {
        Locked_Node_Data locked_node_data(this);
        output_tokens = locked_node_data.get_output_tokens();
        assert(locked_node_data.get_outputs().find(output) == locked_node_data.get_outputs().end());
        locked_node_data.modify_outputs().insert(output);
      }

      const auto sft = shared_from_this();
      for (auto &output_token : output_tokens)
        network->get_Job_Queue()->give(std::make_shared<Raven_Token_Insert>(output, network, sft, output_token));
    }

    void Node::disconnect_output(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output) {
      Tokens output_tokens;

      {
        Locked_Node_Data locked_node_data(this);
        output_tokens = locked_node_data.get_output_tokens();
        const auto found = locked_node_data.get_outputs().find(output);
        assert(found != locked_node_data.get_outputs().end());
        locked_node_data.modify_outputs().erase(found);
        --locked_node_data.modify_output_count();
        assert(locked_node_data.get_output_count() >= 0);
        if (locked_node_data.get_output_count() == 0)
          send_disconnect_from_parents(network, locked_node_data);
      }

      const auto sft = shared_from_this();
      for (auto &output_token : output_tokens)
        network->get_Job_Queue()->give(std::make_shared<Raven_Token_Remove>(output, network, sft, output_token));
    }

    void Node::receive(Concurrency::Job_Queue &job_queue, const Concurrency::Raven &raven) {
      if (const Raven_Disconnect_Output * const rdo = dynamic_cast<const Raven_Disconnect_Output *>(&raven))
        disconnect_output(rdo->get_Network(), rdo->get_output());
      else if (const Raven_Token * const rt = dynamic_cast<const Raven_Token *>(&raven))
        rt->receive();
      else
        abort();
    }

    void Node::receive(const Raven_Disconnect_Output &raven) {
      Locked_Node_Data locked_node_data(this);

      const auto found = locked_node_data.get_outputs().find(raven.get_output());
      assert(found != locked_node_data.get_outputs().end());
      locked_node_data.modify_outputs().erase(found);
      --locked_node_data.modify_output_count();
      assert(locked_node_data.get_output_count() >= 0);
      if (locked_node_data.get_output_count() == 0)
        send_disconnect_from_parents(raven.get_Network(), locked_node_data);
    }

  }

}
