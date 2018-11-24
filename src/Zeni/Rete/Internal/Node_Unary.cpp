#include "Zeni/Rete/Internal/Node_Unary.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Internal/Job_Sequential_Messages.hpp"
#include "Zeni/Rete/Internal/Message_Disconnect_Output.hpp"
#include "Zeni/Rete/Network.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Unary::Node_Unary(const int64_t height, const int64_t size, const int64_t token_size, const size_t hash, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> input)
    : Node(height, size, token_size, hash),
    m_key(key),
    m_input(input)
  {
  }

  void Node_Unary::connect_to_parents_again(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) {
    const auto sft = shared_from_this();

    if (const auto multisym = std::dynamic_pointer_cast<const Node_Key_Multisym>(m_key)) {
      for (auto sym : multisym->symbols)
        m_input->connect_existing_output(network, job_queue, sym, sft, false);
    }
    else
      m_input->connect_existing_output(network, job_queue, m_key, sft, false);
  }

  void Node_Unary::send_disconnect_from_parents(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) {
    if (const auto multisym = std::dynamic_pointer_cast<const Node_Key_Multisym>(m_key)) {
      auto mt = multisym->symbols.cbegin();
      const auto last_message = std::make_shared<Message_Disconnect_Output>(m_input, network, *mt++, shared_from_this());
      const auto counter = std::make_shared<std::atomic_int64_t>(int64_t(multisym->symbols.size()) - 1);
      for (const auto mend = multisym->symbols.cend(); mt != mend; ++mt) {
        job_queue->give_one(std::make_shared<Job_Sequential_Messages_Countdown>(network,
          std::make_shared<Message_Disconnect_Output>(m_input, network, *mt, shared_from_this()),
          last_message,
          counter));
      }
    }
    else
      job_queue->give_one(std::make_shared<Message_Disconnect_Output>(m_input, network, m_key, shared_from_this()));
  }

  std::shared_ptr<const Node_Key> Node_Unary::get_key() const {
    return m_key;
  }

  std::shared_ptr<const Node> Node_Unary::get_input() const {
    return m_input;
  }

  std::shared_ptr<Node> Node_Unary::get_input() {
    return m_input;
  }

  bool Node_Unary::is_linked(const std::shared_ptr<Node>, const std::shared_ptr<const Node_Key>) {
    abort();
  }

}
