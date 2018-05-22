#include "Zeni/Rete/Node.hpp"

#include "Zeni/Rete/Network.hpp"
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
      : m_height(height), m_size(size), m_token_size(token_size)
    {
    }

    ZENI_RETE_LINKAGE const Node::Outputs & Node::get_outputs() const {
      return m_outputs;
    }

    ZENI_RETE_LINKAGE int64_t Node::get_height() const { 
      return m_height;
    }

    ZENI_RETE_LINKAGE int64_t Node::get_size() const {
      return m_size;
    }

    ZENI_RETE_LINKAGE int64_t Node::get_token_size() const {
      return m_token_size;
    }

    void Node::connect_output(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output) {
      Tokens output_tokens;

      {
        Concurrency::Mutex::Lock lock(m_mutex);
        output_tokens = m_output_tokens;
        assert(m_outputs.find(output) == m_outputs.end());
        m_outputs.insert(output);
      }

      const auto sft = shared_from_this();
      for (auto &output_token : output_tokens)
        network->get_Job_Queue()->give(std::make_shared<Raven_Token_Insert>(output, network, sft, output_token));
    }

    void Node::disconnect_output(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output) {
      Tokens output_tokens;

      {
        Concurrency::Mutex::Lock lock(m_mutex);
        output_tokens = m_output_tokens;
        const auto found = m_outputs.find(output);
        assert(found != m_outputs.end());
        m_outputs.erase(found);
      }

      const auto sft = shared_from_this();
      for (auto &output_token : output_tokens)
        network->get_Job_Queue()->give(std::make_shared<Raven_Token_Remove>(output, network, sft, output_token));
    }

    void Node::receive(Concurrency::Job_Queue &job_queue, const Concurrency::Raven &raven) {
      dynamic_cast<const Raven_Token &>(raven).receive();
    }

  }

}
