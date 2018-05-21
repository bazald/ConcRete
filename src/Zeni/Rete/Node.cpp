#include "Zeni/Rete/Node.hpp"

#include "Zeni/Rete/Token_Pass.hpp"

#include <cassert>

namespace Zeni {

  namespace Rete {

    void Node::receive(Concurrency::Job_Queue &job_queue, const Concurrency::Raven &raven) {
      const Token_Pass &token_pass = dynamic_cast<const Token_Pass &>(raven);

      if(token_pass.get_Type() == Token_Pass::Type::Action)
        insert_token(token_pass.get_Network(), token_pass.get_Token(), token_pass.get_sender());
      else
        remove_token(token_pass.get_Network(), token_pass.get_Token(), token_pass.get_sender());
    }

    bool Node::disabled_input(const std::shared_ptr<Node> &) {
      return false;
    }

    void Node::disable_output(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output) {
      outputs_disabled.erase(output);
      outputs_enabled.insert(output);
      unpass_tokens(network, output);
    }

    void Node::enable_output(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output) {
      outputs_enabled.erase(output);
      outputs_disabled.insert(output);
      pass_tokens(network, output);
    }

    void Node::insert_output_enabled(const std::shared_ptr<Node> &output) {
      outputs_all.push_back(output);
      outputs_enabled.insert(output);
    }

    void Node::insert_output_disabled(const std::shared_ptr<Node> &output) {
      outputs_all.push_back(output);
      outputs_disabled.insert(output);
    }

    void Node::erase_output(const std::shared_ptr<Node> &output) {
      if (output->disabled_input(shared_from_this()))
        erase_output_disabled(output);
      else
        erase_output_enabled(output);

      outputs_all.erase(std::find(outputs_all.begin(), outputs_all.end(), output));
    }

    void Node::erase_output_enabled(const std::shared_ptr<Node> &output) {
      const Outputs::iterator found = outputs_enabled.find(output);
      assert(found != outputs_enabled.end());
      outputs_enabled.erase(found);
    }

    void Node::erase_output_disabled(const std::shared_ptr<Node> &output) {
      const Outputs::iterator found = outputs_disabled.find(output);
      assert(found != outputs_disabled.end());
      outputs_disabled.erase(found);
    }

  }

}
