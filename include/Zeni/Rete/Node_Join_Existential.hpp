#ifndef ZENI_RETE_NODE_JOIN_EXISTENTIAL_HPP
#define ZENI_RETE_NODE_JOIN_EXISTENTIAL_HPP

#include "Internal/Node_Binary.hpp"

namespace Zeni::Rete {

  class Node_Join_Existential : public Node_Binary {
    Node_Join_Existential(const Node_Join_Existential &) = delete;
    Node_Join_Existential & operator=(const Node_Join_Existential &) = delete;

    Node_Join_Existential(const std::shared_ptr<const Node_Key> node_key_left, const std::shared_ptr<const Node_Key> node_key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const Variable_Bindings &variable_bindings);
    Node_Join_Existential(const std::shared_ptr<const Node_Key> node_key_left, const std::shared_ptr<const Node_Key> node_key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, Variable_Bindings &&variable_bindings);

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

    ZENI_RETE_LINKAGE static std::shared_ptr<Node_Join_Existential> Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> node_key_left, const std::shared_ptr<const Node_Key> node_key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const Variable_Bindings &variable_bindings);
    ZENI_RETE_LINKAGE static std::shared_ptr<Node_Join_Existential> Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> node_key_left, const std::shared_ptr<const Node_Key> node_key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, Variable_Bindings &&variable_bindings);

    ZENI_RETE_LINKAGE std::pair<Node_Trie::Result, std::shared_ptr<Node>> connect_new_or_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) override;
    ZENI_RETE_LINKAGE Node_Trie::Result connect_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) override;

    ZENI_RETE_LINKAGE Node_Trie::Result link_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) override;
    ZENI_RETE_LINKAGE Node_Trie::Result unlink_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) override;

    ZENI_RETE_LINKAGE bool has_tokens(const std::shared_ptr<const Node_Key> key) const override;

    ZENI_RETE_LINKAGE void receive(const Message_Token_Insert &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Token_Remove &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Connect_Join &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Disconnect_Output &message) override;

    ZENI_RETE_LINKAGE bool operator==(const Node &rhs) const override;

  private:
    static std::shared_ptr<Node_Join_Existential> connect_created(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const std::shared_ptr<Node_Join_Existential> created);
    std::shared_ptr<Symbols> bind_variables_left(const std::shared_ptr<const Token> token_left) const;
    std::shared_ptr<Symbols> bind_variables_right(const std::shared_ptr<const Token> token_right) const;

    Join_Layer_Trie m_join_layer_trie;

    const Variable_Bindings m_variable_bindings;
  };

}

#endif
