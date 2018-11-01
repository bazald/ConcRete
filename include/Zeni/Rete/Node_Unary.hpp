#ifndef ZENI_RETE_NODE_UNARY_HPP
#define ZENI_RETE_NODE_UNARY_HPP

#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  class Node_Unary : public Node {
    Node_Unary(const Node_Unary &) = delete;
    Node_Unary & operator=(const Node_Unary &) = delete;

  protected:
    Node_Unary(const int64_t height, const int64_t size, const int64_t token_size, const size_t hash, const std::shared_ptr<Node> input);

    ZENI_RETE_LINKAGE void send_disconnect_from_parents(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) override;

  public:
    ZENI_RETE_LINKAGE std::pair<std::shared_ptr<Node>, std::shared_ptr<Node>> get_inputs() override;

    ZENI_RETE_LINKAGE std::shared_ptr<const Node> get_input() const;
    ZENI_RETE_LINKAGE std::shared_ptr<Node> get_input();

  private:
    const std::shared_ptr<Node> m_input;
  };

}

#endif
