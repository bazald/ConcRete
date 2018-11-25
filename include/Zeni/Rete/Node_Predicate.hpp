#ifndef ZENI_RETE_NODE_PREDICATE_HPP
#define ZENI_RETE_NODE_PREDICATE_HPP

#include "Internal/Node_Unary.hpp"

namespace Zeni::Rete {

  class Node_Predicate : public Node_Unary {
    Node_Predicate(const Node_Predicate &) = delete;
    Node_Predicate & operator=(const Node_Predicate &) = delete;

  public:
    typedef Concurrency::Super_Hash_Trie<Token_Trie, Node_Trie, Node_Trie> Predicate_Layer_Trie;
    typedef Predicate_Layer_Trie::Snapshot Predicate_Layer_Snapshot;
    enum Filter_Layer_2 {
      PREDICATE_LAYER_TOKENS = 0,
      PREDICATE_LAYER_OUTPUTS = 1,
      PREDICATE_LAYER_OUTPUTS_UNLINKED = 2
    };

    class Predicate : std::enable_shared_from_this<Predicate> {
      Predicate(const Predicate &) = delete;
      Predicate & operator =(const Predicate &) = delete;

    protected:
      ZENI_RETE_LINKAGE Predicate() {}

    public:
      ZENI_RETE_LINKAGE virtual ~Predicate() {}

      ZENI_RETE_LINKAGE virtual bool operator()(const std::shared_ptr<const Symbol> lhs, const std::shared_ptr<const Symbol> rhs) const = 0;
    };

    class Predicate_E : public Predicate {
      Predicate_E(const Predicate_E &) = delete;
      Predicate_E & operator =(const Predicate_E &) = delete;

      Predicate_E() {}

    public:
      ZENI_RETE_LINKAGE static std::shared_ptr<const Predicate_E> Create();

      ZENI_RETE_LINKAGE bool operator()(const std::shared_ptr<const Symbol> lhs, const std::shared_ptr<const Symbol> rhs) const;
    };

    class Predicate_NE : public Predicate {
      Predicate_NE(const Predicate_NE &) = delete;
      Predicate_NE & operator =(const Predicate_NE &) = delete;

      Predicate_NE() {}

    public:
      ZENI_RETE_LINKAGE static std::shared_ptr<const Predicate_NE> Create();

      ZENI_RETE_LINKAGE bool operator()(const std::shared_ptr<const Symbol> lhs, const std::shared_ptr<const Symbol> rhs) const;
    };

    class Predicate_GT : public Predicate {
      Predicate_GT(const Predicate_GT &) = delete;
      Predicate_GT & operator =(const Predicate_GT &) = delete;

      Predicate_GT() {}

    public:
      ZENI_RETE_LINKAGE static std::shared_ptr<const Predicate_GT> Create();

      ZENI_RETE_LINKAGE bool operator()(const std::shared_ptr<const Symbol> lhs, const std::shared_ptr<const Symbol> rhs) const;
    };

    class Predicate_GTE : public Predicate {
      Predicate_GTE(const Predicate_GTE &) = delete;
      Predicate_GTE & operator =(const Predicate_GTE &) = delete;

      Predicate_GTE() {}

    public:
      ZENI_RETE_LINKAGE static std::shared_ptr<const Predicate_GTE> Create();

      ZENI_RETE_LINKAGE bool operator()(const std::shared_ptr<const Symbol> lhs, const std::shared_ptr<const Symbol> rhs) const;
    };

    class Predicate_LT : public Predicate {
      Predicate_LT(const Predicate_LT &) = delete;
      Predicate_LT & operator =(const Predicate_LT &) = delete;

      Predicate_LT() {}

    public:
      ZENI_RETE_LINKAGE static std::shared_ptr<const Predicate_LT> Create();

      ZENI_RETE_LINKAGE bool operator()(const std::shared_ptr<const Symbol> lhs, const std::shared_ptr<const Symbol> rhs) const;
    };

    class Predicate_LTE : public Predicate {
      Predicate_LTE(const Predicate_LTE &) = delete;
      Predicate_LTE & operator =(const Predicate_LTE &) = delete;

      Predicate_LTE() {}

    public:
      ZENI_RETE_LINKAGE static std::shared_ptr<const Predicate_LTE> Create();

      ZENI_RETE_LINKAGE bool operator()(const std::shared_ptr<const Symbol> lhs, const std::shared_ptr<const Symbol> rhs) const;
    };

    class Predicate_STA : public Predicate {
      Predicate_STA(const Predicate_STA &) = delete;
      Predicate_STA & operator =(const Predicate_STA &) = delete;

      Predicate_STA() {}

    public:
      ZENI_RETE_LINKAGE static std::shared_ptr<const Predicate_STA> Create();

      ZENI_RETE_LINKAGE bool operator()(const std::shared_ptr<const Symbol> lhs, const std::shared_ptr<const Symbol> rhs) const;
    };

    class Get_Symbol : std::enable_shared_from_this<Get_Symbol> {
      Get_Symbol(const Get_Symbol &) = delete;
      Get_Symbol & operator =(const Get_Symbol &) = delete;

    protected:
      ZENI_RETE_LINKAGE Get_Symbol() {}

    public:
      ZENI_RETE_LINKAGE virtual ~Get_Symbol() {}

      ZENI_RETE_LINKAGE virtual std::shared_ptr<const Symbol> get_symbol(const std::shared_ptr<const Token> token) const = 0;
    };

    class Get_Symbol_Constant : public Get_Symbol {
      Get_Symbol_Constant(const Get_Symbol_Constant &) = delete;
      Get_Symbol_Constant & operator =(const Get_Symbol_Constant &) = delete;

    public:
      ZENI_RETE_LINKAGE Get_Symbol_Constant(const std::shared_ptr<const Symbol> symbol_) : symbol(symbol_) {}

      ZENI_RETE_LINKAGE std::shared_ptr<const Symbol> get_symbol(const std::shared_ptr<const Token> token) const override;

      const std::shared_ptr<const Symbol> symbol;
    };

    class Get_Symbol_Variable : public Get_Symbol {
      Get_Symbol_Variable(const Get_Symbol_Variable &) = delete;
      Get_Symbol_Variable & operator =(const Get_Symbol_Variable &) = delete;

    public:
      ZENI_RETE_LINKAGE Get_Symbol_Variable(const Token_Index index_) : index(index_) {}

      ZENI_RETE_LINKAGE std::shared_ptr<const Symbol> get_symbol(const std::shared_ptr<const Token> token) const override;

      const Token_Index index;
    };

  private:
    Node_Predicate(const std::shared_ptr<Network> network, const std::shared_ptr<const Node_Key> node_key, const std::shared_ptr<Node> input, const std::shared_ptr<const Predicate> predicate, const Token_Index &lhs, const std::shared_ptr<const Get_Symbol> rhs);

  public:
    ZENI_RETE_LINKAGE static std::shared_ptr<Node_Predicate> Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> node_key, const std::shared_ptr<Node> input, const std::shared_ptr<const Predicate> predicate, const Token_Index &lhs, const std::shared_ptr<const Get_Symbol> rhs);

    ZENI_RETE_LINKAGE std::pair<Node_Trie::Result, std::shared_ptr<Node>> connect_new_or_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) override;
    ZENI_RETE_LINKAGE Node_Trie::Result connect_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child, const bool unlinked) override;

    ZENI_RETE_LINKAGE Node_Trie::Result link_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) override;
    ZENI_RETE_LINKAGE Node_Trie::Result unlink_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) override;

    ZENI_RETE_LINKAGE bool has_tokens(const std::shared_ptr<const Node_Key> key) const override;

    ZENI_RETE_LINKAGE void receive(const Message_Token_Insert &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Token_Remove &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Connect_Predicate &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Disconnect_Output &message) override;

    ZENI_RETE_LINKAGE bool operator==(const Node &rhs) const override;

  private:
    ZENI_RETE_LINKAGE void insert_tokens(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> child, const Predicate_Layer_Snapshot snapshot);
    ZENI_RETE_LINKAGE void remove_tokens(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> child, const Predicate_Layer_Snapshot snapshot);

    Predicate_Layer_Trie m_predicate_layer_trie;

    const std::shared_ptr<const Predicate> m_predicate;
    const Token_Index m_lhs_index;
    const std::shared_ptr<const Get_Symbol> m_rhs;
  };

}

#endif
