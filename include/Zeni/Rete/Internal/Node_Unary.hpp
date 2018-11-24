#ifndef ZENI_RETE_NODE_UNARY_HPP
#define ZENI_RETE_NODE_UNARY_HPP

#include "Zeni/Rete/Node_Key.hpp"
#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  class Node_Unary : public Node {
    Node_Unary(const Node_Unary &) = delete;
    Node_Unary & operator=(const Node_Unary &) = delete;

  protected:
    Node_Unary(const int64_t height, const int64_t size, const int64_t token_size, const size_t hash, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> input);

  public:
    ZENI_RETE_LINKAGE void connect_to_parents_again(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) override;
    ZENI_RETE_LINKAGE void send_disconnect_from_parents(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) override;

    ZENI_RETE_LINKAGE std::shared_ptr<const Node_Key> get_key() const;
    ZENI_RETE_LINKAGE std::shared_ptr<const Node> get_input() const;
    ZENI_RETE_LINKAGE std::shared_ptr<Node> get_input();

    ZENI_RETE_LINKAGE bool is_linked(const std::shared_ptr<Node> input, const std::shared_ptr<const Node_Key> key) override;

  private:
    const std::shared_ptr<const Node_Key> m_key;
    const std::shared_ptr<Node> m_input;
  };

}

#endif
