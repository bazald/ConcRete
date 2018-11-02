#ifndef ZENI_RETE_NODE_BINARY_HPP
#define ZENI_RETE_NODE_BINARY_HPP

#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  class Node_Binary : public Node {
    Node_Binary(const Node_Binary &) = delete;
    Node_Binary & operator=(const Node_Binary &) = delete;

  protected:
    Node_Binary(const int64_t height, const int64_t size, const int64_t token_size, const size_t hash, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right);

    ZENI_RETE_LINKAGE void send_disconnect_from_parents(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) override;

  public:
    ZENI_RETE_LINKAGE std::pair<std::shared_ptr<Node>, std::shared_ptr<Node>> get_inputs() override;

    ZENI_RETE_LINKAGE std::shared_ptr<const Node> get_input_left() const;
    ZENI_RETE_LINKAGE std::shared_ptr<Node> get_input_left();
    ZENI_RETE_LINKAGE std::shared_ptr<const Node> get_input_right() const;
    ZENI_RETE_LINKAGE std::shared_ptr<Node> get_input_right();

  private:
    const std::shared_ptr<Node> m_input_left;
    const std::shared_ptr<Node> m_input_right;
  };

}

#endif
