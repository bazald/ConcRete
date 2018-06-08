#ifndef ZENI_RETE_NODE_HPP
#define ZENI_RETE_NODE_HPP

#include "Zeni/Concurrency/Atomic.hpp"
#include "Zeni/Concurrency/Recipient.hpp"
#include "Custom_Data.hpp"
#include "Token.hpp"

namespace Zeni::Rete {

  class Custom_Data;
  class Network;
  class Message_Connect_Gate;
  class Message_Connect_Output;
  class Message_Disconnect_Gate;
  class Message_Decrement_Output_Count;
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
    struct Outputs {
      std::unordered_multiset<std::shared_ptr<Node>> positive;
      std::unordered_multiset<std::shared_ptr<Node>> negative;
    };

    class Unlocked_Node_Data;
    class Locked_Node_Data_Const;
    class Locked_Node_Data;

  protected:
    ZENI_RETE_LINKAGE std::shared_ptr<const Node> shared_from_this() const;
    ZENI_RETE_LINKAGE std::shared_ptr<Node> shared_from_this();

    ZENI_RETE_LINKAGE Node(const int64_t height, const int64_t size, const int64_t token_size);

    ZENI_RETE_LINKAGE virtual void send_disconnect_from_parents(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) = 0;

  public:
    class Unlocked_Node_Data {
      Unlocked_Node_Data(const Unlocked_Node_Data &) = delete;
      Unlocked_Node_Data & operator=(const Unlocked_Node_Data &) = delete;

      friend Locked_Node_Data_Const;
      friend Locked_Node_Data;

    public:
      Unlocked_Node_Data();

    private:
      Outputs m_gates;
      Outputs m_outputs;
      Tokens_Output m_output_tokens;
    };

    class Locked_Node_Data_Const {
      Locked_Node_Data_Const(const Locked_Node_Data_Const &) = delete;
      Locked_Node_Data_Const & operator=(const Locked_Node_Data_Const &) = delete;

      friend Locked_Node_Data;

    public:
      ZENI_RETE_LINKAGE Locked_Node_Data_Const(const Node * node);

      ZENI_RETE_LINKAGE const Outputs & get_gates() const;
      ZENI_RETE_LINKAGE const Outputs & get_outputs() const;
      ZENI_RETE_LINKAGE const Tokens_Output & get_output_tokens() const;

    private:
      const Concurrency::Mutex::Lock m_lock;
      const std::shared_ptr<const Unlocked_Node_Data> m_data;
    };

    class Locked_Node_Data : public Locked_Node_Data_Const {
      Locked_Node_Data(const Locked_Node_Data &) = delete;
      Locked_Node_Data & operator=(const Locked_Node_Data &) = delete;

    public:
      ZENI_RETE_LINKAGE Locked_Node_Data(Node * node);

      ZENI_RETE_LINKAGE Outputs & modify_gates();
      ZENI_RETE_LINKAGE Outputs & modify_outputs();
      ZENI_RETE_LINKAGE Tokens_Output & modify_output_tokens();

    private:
      const std::shared_ptr<Unlocked_Node_Data> m_data;
    };

    ZENI_RETE_LINKAGE int64_t get_height() const;
    ZENI_RETE_LINKAGE int64_t get_size() const;
    ZENI_RETE_LINKAGE int64_t get_token_size() const;

    ZENI_RETE_LINKAGE virtual std::pair<std::shared_ptr<Node>, std::shared_ptr<Node>> get_inputs() = 0;

    /// Increments the child count if greater than 0 and returns true, otherwise returns false.
    ZENI_RETE_LINKAGE bool try_increment_child_count();
    /// Finds an existing equivalent to output and return it, or returns the new output if no equivalent exists.
    ZENI_RETE_LINKAGE std::shared_ptr<Node> connect_gate(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> output);
    /// Finds an existing equivalent to output and return it, or returns the new output if no equivalent exists.
    ZENI_RETE_LINKAGE std::shared_ptr<Node> connect_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> output);

    ZENI_RETE_LINKAGE void receive(const std::shared_ptr<const Concurrency::Message> message) noexcept override;
    ZENI_RETE_LINKAGE virtual void receive(const Message_Connect_Gate &message);
    ZENI_RETE_LINKAGE virtual void receive(const Message_Connect_Output &message);
    ZENI_RETE_LINKAGE void receive(const Message_Decrement_Output_Count &message);
    ZENI_RETE_LINKAGE virtual void receive(const Message_Disconnect_Gate &message);
    ZENI_RETE_LINKAGE virtual void receive(const Message_Disconnect_Output &message);
    ZENI_RETE_LINKAGE virtual void receive(const Message_Status_Empty &message) = 0;
    ZENI_RETE_LINKAGE virtual void receive(const Message_Status_Nonempty &message) = 0;
    ZENI_RETE_LINKAGE virtual void receive(const Message_Token_Insert &message) = 0;
    ZENI_RETE_LINKAGE virtual void receive(const Message_Token_Remove &message) = 0;

    ZENI_RETE_LINKAGE virtual bool operator==(const Node &rhs) const = 0;

    protected:
    /// Returns true if the first instance of the sender gate has been inserted
    ZENI_RETE_LINKAGE virtual bool receive(const Message_Connect_Gate &message, Locked_Node_Data &locked_node_data);
    /// Returns true if the first instance of the sender output has been inserted
    ZENI_RETE_LINKAGE virtual bool receive(const Message_Connect_Output &message, Locked_Node_Data &locked_node_data);
    /// Returns true if the last instance of the sender gate has been removed
    ZENI_RETE_LINKAGE virtual bool receive(const Message_Disconnect_Gate &message, Locked_Node_Data &locked_node_data);
    /// Returns true if the last instance of the sender output has been removed
    ZENI_RETE_LINKAGE virtual bool receive(const Message_Disconnect_Output &message, Locked_Node_Data &locked_node_data);

  private:
    int64_t decrement_child_count();

    const int64_t m_height;
    const int64_t m_size;
    const int64_t m_token_size;

    Concurrency::Atomic_int64_t<false> m_child_count = 1;

    std::shared_ptr<Unlocked_Node_Data> m_unlocked_node_data;

  public:
    std::shared_ptr<Custom_Data> custom_data;
  };

}

#endif
