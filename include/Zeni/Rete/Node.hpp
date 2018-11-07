#ifndef ZENI_RETE_NODE_HPP
#define ZENI_RETE_NODE_HPP

#include "Zeni/Concurrency/Container/Antiable_Hash_Trie.hpp"
#include "Zeni/Concurrency/Container/Positive_Hash_Trie.hpp"
#include "Zeni/Concurrency/Atomic.hpp"
#include "Zeni/Concurrency/Recipient.hpp"
#include "Custom_Data.hpp"
#include "Token.hpp"

namespace Zeni::Rete {

  class Custom_Data;
  class Message_Connect_Filter_0;
  class Message_Connect_Filter_1;
  class Message_Connect_Filter_2;
  class Message_Disconnect_Output;
  class Message_Token_Insert;
  class Message_Token_Remove;
  class Network;
  class Node_Key;

  class Node : public Concurrency::Recipient
  {
    Node(const Node &) = delete;
    Node & operator=(const Node &) = delete;

  public:
    typedef Concurrency::Antiable_Hash_Trie<std::shared_ptr<const Token>, hash_deref<Token>, compare_deref_eq> Token_Trie;
    typedef Concurrency::Positive_Hash_Trie<std::shared_ptr<Node>, hash_deref<Node>, compare_deref_eq> Node_Trie;

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
    ZENI_RETE_LINKAGE virtual std::shared_ptr<const Node_Key> get_key_for_input(const std::shared_ptr<const Node> input) const = 0;

    /// Finds an existing equivalent to output and return it, or returns the new output if no equivalent exists.
    ZENI_RETE_LINKAGE virtual std::pair<Node_Trie::Result, std::shared_ptr<Node>> connect_new_or_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> child) = 0;
    /// Initiate reconnection of an existing gate.
    ZENI_RETE_LINKAGE virtual Node_Trie::Result connect_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> child) = 0;

    ZENI_RETE_LINKAGE void receive(const std::shared_ptr<const Concurrency::Message> message) noexcept override;
    ZENI_RETE_LINKAGE virtual void receive(const Message_Token_Insert &message) = 0;
    ZENI_RETE_LINKAGE virtual void receive(const Message_Token_Remove &message) = 0;
    ZENI_RETE_LINKAGE virtual void receive(const Message_Connect_Filter_0 &message);
    ZENI_RETE_LINKAGE virtual void receive(const Message_Connect_Filter_1 &message);
    ZENI_RETE_LINKAGE virtual void receive(const Message_Connect_Filter_2 &message);
    ZENI_RETE_LINKAGE virtual void receive(const Message_Disconnect_Output &message) = 0;

    ZENI_RETE_LINKAGE virtual size_t get_hash() const;

    ZENI_RETE_LINKAGE virtual bool operator==(const Node &rhs) const = 0;

  private:
    const int64_t m_height;
    const int64_t m_size;
    const int64_t m_token_size;
    const size_t m_hash;

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
