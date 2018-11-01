#ifndef ZENI_RETE_NODE_HPP
#define ZENI_RETE_NODE_HPP

#include "Zeni/Concurrency/Container/Antiable_Hash_Trie.hpp"
#include "Zeni/Concurrency/Container/Positive_Hash_Trie.hpp"
#include "Zeni/Concurrency/Container/Super_Hash_Trie.hpp"
#include "Zeni/Concurrency/Atomic.hpp"
#include "Zeni/Concurrency/Recipient.hpp"
#include "Internal/Antiable_Set.hpp"
#include "Custom_Data.hpp"
#include "Token.hpp"

namespace Zeni::Rete {

  class Custom_Data;
  class Network;
  class Message_Connect_Gate;
  class Message_Connect_Output;
  class Message_Disconnect_Gate;
  class Message_Disconnect_Output;
  class Message_Status_Empty;
  class Message_Status_Nonempty;
  class Message_Token_Insert;
  class Message_Token_Remove;

  class Node : public Concurrency::Recipient
  {
    Node(const Node &) = delete;
    Node & operator=(const Node &) = delete;

  public:
    typedef Concurrency::Positive_Hash_Trie<std::shared_ptr<Node>, hash_deref<Node>, compare_deref_eq> Node_Trie;
    typedef Concurrency::Antiable_Hash_Trie<std::shared_ptr<const Token>, hash_deref<Token>, compare_deref_eq> Output_Token_Trie;
    typedef Concurrency::Antiable_Hash_Trie<std::shared_ptr<const Token>> Input_Token_Trie;
    typedef Concurrency::Super_Hash_Trie <
      Node_Trie,
      Node_Trie,
      Output_Token_Trie,
      Input_Token_Trie,
      Input_Token_Trie> Node_Data;
    typedef Node_Data::Snapshot Node_Data_Snapshot;
    enum Node_Data_Subtrie {
      NODE_DATA_SUBTRIE_GATES = 0,
      NODE_DATA_SUBTRIE_OUTPUTS = 1,
      NODE_DATA_SUBTRIE_TOKEN_OUTPUTS = 2,
      NODE_DATA_SUBTRIE_TOKEN_INPUTS_LEFT = 3,
      NODE_DATA_SUBTRIE_TOKEN_INPUTS_RIGHT = 4
    };

  protected:
    ZENI_RETE_LINKAGE std::shared_ptr<const Node> shared_from_this() const;
    ZENI_RETE_LINKAGE std::shared_ptr<Node> shared_from_this();

    ZENI_RETE_LINKAGE Node(const int64_t height, const int64_t size, const int64_t token_size, const size_t hash);

  public:
    ZENI_RETE_LINKAGE virtual void send_disconnect_from_parents(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) = 0;

    ZENI_RETE_LINKAGE int64_t get_height() const;
    ZENI_RETE_LINKAGE int64_t get_size() const;
    ZENI_RETE_LINKAGE int64_t get_token_size() const;

    ZENI_RETE_LINKAGE virtual std::pair<std::shared_ptr<Node>, std::shared_ptr<Node>> get_inputs() = 0;

    /// Finds an existing equivalent to output and return it, or returns the new output if no equivalent exists.
    ZENI_RETE_LINKAGE std::shared_ptr<Node> connect_new_or_existing_gate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> child);
    /// Finds an existing equivalent to output and return it, or returns the new output if no equivalent exists.
    ZENI_RETE_LINKAGE std::shared_ptr<Node> connect_new_or_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> child);
    /// Initiate reconnection of an existing gate.
    ZENI_RETE_LINKAGE void connect_existing_gate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> child);
    /// Initiate reconnection of an existing output.
    ZENI_RETE_LINKAGE void connect_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> child);

    ZENI_RETE_LINKAGE void receive(const std::shared_ptr<const Concurrency::Message> message) noexcept override;
    /// Returns true if there exist tokens to pass to the sender gate, otherwise false
    ZENI_RETE_LINKAGE virtual bool receive(const Message_Connect_Gate &message);
    /// Returns true if there exist tokens to retract from the sender output, otherwise false
    ZENI_RETE_LINKAGE virtual bool receive(const Message_Connect_Output &message);
    /// Returns true if there exist tokens to pass to the sender gate, otherwise false
    ZENI_RETE_LINKAGE virtual bool receive(const Message_Disconnect_Gate &message);
    /// Returns true if there exist tokens to retract from the sender output, otherwise false
    ZENI_RETE_LINKAGE virtual bool receive(const Message_Disconnect_Output &message);
    ZENI_RETE_LINKAGE virtual void receive(const Message_Status_Empty &message) = 0;
    ZENI_RETE_LINKAGE virtual void receive(const Message_Status_Nonempty &message) = 0;
    ZENI_RETE_LINKAGE virtual void receive(const Message_Token_Insert &message) = 0;
    ZENI_RETE_LINKAGE virtual void receive(const Message_Token_Remove &message) = 0;

    ZENI_RETE_LINKAGE virtual size_t get_hash() const;

    ZENI_RETE_LINKAGE virtual bool operator==(const Node &rhs) const = 0;

  private:
    const int64_t m_height;
    const int64_t m_size;
    const int64_t m_token_size;
    const size_t m_hash;

  protected:
    Node_Data m_node_data;

  public:
    std::shared_ptr<Custom_Data> custom_data;
  };

}

namespace std {
  template <> struct hash<Zeni::Rete::Node> {
    size_t operator()(const Zeni::Rete::Node &node) const {
      return node.get_hash();
    }
  };
}

#endif
