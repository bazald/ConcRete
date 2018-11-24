#ifndef ZENI_RETE_NODE_BINARY_HPP
#define ZENI_RETE_NODE_BINARY_HPP

#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  class Node_Binary : public Node {
    Node_Binary(const Node_Binary &) = delete;
    Node_Binary & operator=(const Node_Binary &) = delete;

  protected:
    Node_Binary(const int64_t height, const int64_t size, const int64_t token_size, const size_t hash, const std::shared_ptr<const Node_Key> key_left, const std::shared_ptr<const Node_Key> key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const Variable_Bindings &variable_bindings);
    Node_Binary(const int64_t height, const int64_t size, const int64_t token_size, const size_t hash, const std::shared_ptr<const Node_Key> key_left, const std::shared_ptr<const Node_Key> key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, Variable_Bindings &&variable_bindings);

  public:
    typedef Concurrency::Super_Hash_Trie<Symbols_Trie, Output_Token_Trie, Node_Trie, Node_Trie> Join_Layer_Trie;
    typedef Join_Layer_Trie::Snapshot Join_Layer_Snapshot;
    enum Join_Layer {
      JOIN_LAYER_SYMBOLS = 0,
      JOIN_LAYER_OUTPUT_TOKENS = 1,
      JOIN_LAYER_OUTPUTS = 2,
      JOIN_LAYER_OUTPUTS_UNLINKED = 3
    };
    enum Join_Layer_Symbol {
      JOIN_LAYER_SYMBOLS_TOKENS_LEFT = 0,
      JOIN_LAYER_SYMBOLS_TOKENS_RIGHT = 1
    };

    ZENI_RETE_LINKAGE void connect_to_parents_again(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) override;
    ZENI_RETE_LINKAGE void send_disconnect_from_parents(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) override;

    ZENI_RETE_LINKAGE std::shared_ptr<const Node_Key> get_key_left() const;
    ZENI_RETE_LINKAGE std::shared_ptr<const Node_Key> get_key_right() const;
    ZENI_RETE_LINKAGE std::shared_ptr<const Node> get_input_left() const;
    ZENI_RETE_LINKAGE std::shared_ptr<Node> get_input_left();
    ZENI_RETE_LINKAGE std::shared_ptr<const Node> get_input_right() const;
    ZENI_RETE_LINKAGE std::shared_ptr<Node> get_input_right();

    ZENI_RETE_LINKAGE std::pair<Node_Trie::Result, std::shared_ptr<Node>> connect_new_or_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) override;
    ZENI_RETE_LINKAGE Node_Trie::Result connect_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child, const bool unlinked) override;

    ZENI_RETE_LINKAGE Node_Trie::Result link_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) override;
    ZENI_RETE_LINKAGE Node_Trie::Result unlink_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) override;

    ZENI_RETE_LINKAGE bool has_tokens(const std::shared_ptr<const Node_Key> key) const override;

    ZENI_RETE_LINKAGE void receive(const Message_Connect_Join &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Disconnect_Output &message) override;

    ZENI_RETE_LINKAGE bool is_linked(const std::shared_ptr<Node> input, const std::shared_ptr<const Node_Key> key) override;

  private:
    void insert_tokens(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child, const Join_Layer_Snapshot snapshot);
    void remove_tokens(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child, const Join_Layer_Snapshot snapshot);

    const std::shared_ptr<const Node_Key> m_key_left;
    const std::shared_ptr<const Node_Key> m_key_right;
    const std::shared_ptr<Node> m_input_left;
    const std::shared_ptr<Node> m_input_right;

  protected:
    static std::shared_ptr<Node_Binary> connect_created(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const std::shared_ptr<Node_Binary> created);
    std::shared_ptr<Symbols> bind_variables_left(const std::shared_ptr<const Token> token_left) const;
    std::shared_ptr<Symbols> bind_variables_right(const std::shared_ptr<const Token> token_right) const;

    const bool m_left_eq_right;

    Join_Layer_Trie m_join_layer_trie;

    const Variable_Bindings m_variable_bindings;

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

    Concurrency::Intrusive_Shared_Ptr<Link_Data> m_link_data = m_left_eq_right ? nullptr : new Link_Data();
  };

}

#endif
