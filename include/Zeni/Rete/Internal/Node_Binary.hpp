#ifndef ZENI_RETE_NODE_BINARY_HPP
#define ZENI_RETE_NODE_BINARY_HPP

#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  class Node_Binary : public Node {
    Node_Binary(const Node_Binary &) = delete;
    Node_Binary & operator=(const Node_Binary &) = delete;

  protected:
    Node_Binary(const int64_t height, const int64_t size, const int64_t token_size, const size_t hash, const std::shared_ptr<const Node_Key> key_left, const std::shared_ptr<const Node_Key> key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right);

  public:
    ZENI_RETE_LINKAGE void connect_to_parents_again(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) override;
    ZENI_RETE_LINKAGE void send_disconnect_from_parents(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) override;

    ZENI_RETE_LINKAGE std::shared_ptr<const Node_Key> get_key_left() const;
    ZENI_RETE_LINKAGE std::shared_ptr<const Node_Key> get_key_right() const;
    ZENI_RETE_LINKAGE std::shared_ptr<const Node> get_input_left() const;
    ZENI_RETE_LINKAGE std::shared_ptr<Node> get_input_left();
    ZENI_RETE_LINKAGE std::shared_ptr<const Node> get_input_right() const;
    ZENI_RETE_LINKAGE std::shared_ptr<Node> get_input_right();

    ZENI_RETE_LINKAGE bool is_linked(const std::shared_ptr<Node> input, const std::shared_ptr<const Node_Key> key) override;

  private:
    const std::shared_ptr<const Node_Key> m_key_left;
    const std::shared_ptr<const Node_Key> m_key_right;
    const std::shared_ptr<Node> m_input_left;
    const std::shared_ptr<Node> m_input_right;

  protected:
    enum class Link_Status { BOTH_LINKED, LEFT_UNLINKED, RIGHT_UNLINKED };
    class Link_Data : public Concurrency::Enable_Intrusive_Sharing {
      Link_Data(const Link_Data &) = delete;
      Link_Data & operator=(const Link_Data &) = delete;

    public:
      Link_Data() {}

      Link_Data(const int64_t tokens_left_, const int64_t tokens_right_, const Link_Status link_status_) : tokens_left(tokens_left_), tokens_right(tokens_right_), link_status(link_status_) {}

      const int64_t tokens_left = 0;
      const int64_t tokens_right = 0;
      const Link_Status link_status = Link_Status::RIGHT_UNLINKED;
    };

    Concurrency::Intrusive_Shared_Ptr<Link_Data> m_link_data = new Link_Data();
  };

}

#endif
