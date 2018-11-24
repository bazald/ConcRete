#ifndef ZENI_RETE_NODE_FILTER_1_HPP
#define ZENI_RETE_NODE_FILTER_1_HPP

#include "Internal/Node_Unary.hpp"

namespace Zeni::Rete {

  class Node_Filter_1 : public Node_Unary {
    Node_Filter_1(const Node_Filter_1 &) = delete;
    Node_Filter_1 & operator=(const Node_Filter_1 &) = delete;

    Node_Filter_1(const std::shared_ptr<Network> network, const std::shared_ptr<const Node_Key> node_key);

  public:
    typedef Concurrency::Hash_Trie_S2<std::shared_ptr<const Symbol>, Concurrency::Super_Hash_Trie<Token_Trie, Node_Trie, Node_Trie>, hash_deref<Symbol>, compare_deref_eq> Symbol_Trie;
    typedef Concurrency::Super_Hash_Trie<Symbol_Trie, Node_Trie, Node_Trie> Filter_Layer_1_Trie;
    typedef Filter_Layer_1_Trie::Snapshot Filter_Layer_1_Snapshot;
    enum Filter_Layer_1 {
      FILTER_LAYER_1_SYMBOL = 0,
      FILTER_LAYER_1_VARIABLE_OUTPUTS_01 = 1,
      FILTER_LAYER_1_VARIABLE_OUTPUTS_01_UNLINKED = 2
    };
    enum Filter_Layer_1_Symbol {
      FILTER_LAYER_1_SYMBOL_TOKENS = 0,
      FILTER_LAYER_1_SYMBOL_OUTPUTS = 1,
      FILTER_LAYER_1_SYMBOL_OUTPUTS_UNLINKED = 2
    };

    ZENI_RETE_LINKAGE static std::shared_ptr<Node_Filter_1> Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> node_key);

    ZENI_RETE_LINKAGE std::pair<Node_Trie::Result, std::shared_ptr<Node>> connect_new_or_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) override;
    ZENI_RETE_LINKAGE Node_Trie::Result connect_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child, const bool unlinked) override;

    ZENI_RETE_LINKAGE Node_Trie::Result link_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) override;
    ZENI_RETE_LINKAGE Node_Trie::Result unlink_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) override;

    ZENI_RETE_LINKAGE bool has_tokens(const std::shared_ptr<const Node_Key> key) const override;

    ZENI_RETE_LINKAGE void receive(const Message_Token_Insert &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Token_Remove &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Connect_Filter_1 &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Disconnect_Output &message) override;

    ZENI_RETE_LINKAGE bool operator==(const Node &rhs) const override;

  private:
    ZENI_RETE_LINKAGE void insert_tokens(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child, const Filter_Layer_1_Snapshot snapshot);
    ZENI_RETE_LINKAGE void remove_tokens(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child, const Filter_Layer_1_Snapshot snapshot);

    Filter_Layer_1_Trie m_filter_layer_1_trie;
  };

}

#endif
