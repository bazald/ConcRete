#include "Zeni/Rete/Node_Passthrough.hpp"

#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Raven_Disconnect_Output.hpp"
#include "Zeni/Rete/Raven_Token_Insert.hpp"
#include "Zeni/Rete/Raven_Token_Remove.hpp"
#include "Zeni/Rete/Token_Alpha.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Passthrough::Node_Passthrough(const std::shared_ptr<Node> input)
    : Node_Unary(input->get_height() + 1, input->get_size() + 1, input->get_token_size(), input)
  {
  }

  std::shared_ptr<Node_Passthrough> Node_Passthrough::Create(const std::shared_ptr<Network> network, const std::shared_ptr<Node> input) {
    class Friendly_Node_Passthrough : public Node_Passthrough {
    public:
      Friendly_Node_Passthrough(const std::shared_ptr<Node> &input) : Node_Passthrough(input) {}
    };

    return std::static_pointer_cast<Node_Passthrough>(input->connect_output(network, std::make_shared<Friendly_Node_Passthrough>(input)));
  }

  bool Node_Passthrough::receive(const Raven_Token_Insert &raven) {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::Job>> jobs;
      
    {
      Locked_Node_Data locked_node_data(this);
      Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

      auto found = locked_node_unary_data.get_input_antitokens().find(raven.get_Token());
      if (found == locked_node_unary_data.get_input_antitokens().end()) {
        locked_node_unary_data.modify_input_tokens().emplace(raven.get_Token());
        locked_node_data.modify_output_tokens().emplace(raven.get_Token());
      }
      else {
        locked_node_unary_data.modify_input_antitokens().erase(found);
        return false;
      }

      jobs.reserve(locked_node_data.get_outputs().size());
      for (auto &output : locked_node_data.get_outputs())
        jobs.emplace_back(std::make_shared<Raven_Token_Insert>(output, raven.get_Network(), sft, raven.get_Token()));
    }

    raven.get_Network()->get_Job_Queue()->give_many(std::move(jobs));

    return true;
  }

  bool Node_Passthrough::receive(const Raven_Token_Remove &raven) {
    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::Job>> jobs;

    {
      Locked_Node_Data locked_node_data(this);
      Locked_Node_Unary_Data locked_node_unary_data(this, locked_node_data);

      auto found = locked_node_unary_data.get_input_tokens().find(raven.get_Token());
      if (found != locked_node_unary_data.get_input_tokens().end()) {
        locked_node_unary_data.modify_input_tokens().erase(found);
        locked_node_data.modify_output_tokens().erase(locked_node_data.get_output_tokens().find(raven.get_Token()));
      }
      else {
        locked_node_unary_data.modify_input_antitokens().emplace(raven.get_Token());
        return false;
      }

      jobs.reserve(locked_node_data.get_outputs().size());
      for (auto &output : locked_node_data.get_outputs())
        jobs.emplace_back(std::make_shared<Raven_Token_Remove>(output, raven.get_Network(), sft, raven.get_Token()));
    }

    raven.get_Network()->get_Job_Queue()->give_many(std::move(jobs));

    return true;
  }

  bool Node_Passthrough::operator==(const Node &rhs) const {
    return dynamic_cast<const Node_Passthrough *>(&rhs);
  }

}
