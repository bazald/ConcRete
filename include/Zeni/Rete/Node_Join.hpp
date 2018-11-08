#ifndef ZENI_RETE_NODE_JOIN_HPP
#define ZENI_RETE_NODE_JOIN_HPP

#include "Internal/Node_Binary.hpp"

namespace Zeni::Rete {

  class Node_Join : public Node_Binary {
    Node_Join(const Node_Join &) = delete;
    Node_Join & operator=(const Node_Join &) = delete;

    Node_Join(const std::shared_ptr<const Node_Key> node_key_left, const std::shared_ptr<const Node_Key> node_key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const Variable_Bindings &variable_bindings);
    Node_Join(const std::shared_ptr<const Node_Key> node_key_left, const std::shared_ptr<const Node_Key> node_key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, Variable_Bindings &&variable_bindings);

  public:
    typedef Concurrency::Super_Hash_Trie<Symbols_Trie, Output_Token_Trie, Node_Trie> Join_Layer_Trie;
    typedef Join_Layer_Trie::Snapshot Join_Layer_Snapshot;
    enum Join_Layer {
      JOIN_LAYER_SYMBOLS = 0,
      JOIN_LAYER_OUTPUT_TOKENS = 1,
      JOIN_LAYER_OUTPUTS = 2
    };
    enum Join_Layer_Symbol {
      JOIN_LAYER_SYMBOLS_TOKENS_LEFT = 0,
      JOIN_LAYER_SYMBOLS_TOKENS_RIGHT = 1
    };

    ZENI_RETE_LINKAGE static std::shared_ptr<Node_Join> Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> node_key_left, const std::shared_ptr<const Node_Key> node_key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const Variable_Bindings &variable_bindings);
    ZENI_RETE_LINKAGE static std::shared_ptr<Node_Join> Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> node_key_left, const std::shared_ptr<const Node_Key> node_key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, Variable_Bindings &&variable_bindings);

    ZENI_RETE_LINKAGE std::pair<Node_Trie::Result, std::shared_ptr<Node>> connect_new_or_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) override;
    ZENI_RETE_LINKAGE Node_Trie::Result connect_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) override;

    ZENI_RETE_LINKAGE void receive(const Message_Token_Insert &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Token_Remove &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Connect_Join &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Disconnect_Output &message) override;

    ZENI_RETE_LINKAGE bool operator==(const Node &rhs) const override;

  private:
    static std::shared_ptr<Node_Join> connect_created(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const std::shared_ptr<Node_Join> created);
    std::shared_ptr<Symbols> bind_variables_left(const std::shared_ptr<const Token> token_left) const;
    std::shared_ptr<Symbols> bind_variables_right(const std::shared_ptr<const Token> token_right) const;

    Join_Layer_Trie m_join_layer_trie;

    const Variable_Bindings m_variable_bindings;
  };

}

#endif
