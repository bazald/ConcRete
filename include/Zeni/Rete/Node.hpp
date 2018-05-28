#ifndef ZENI_RETE_NODE_H
#define ZENI_RETE_NODE_H

#include "Zeni/Concurrency/Maester.hpp"
#include "Custom_Data.hpp"
#include "Token.hpp"

namespace Zeni::Rete {

  class Custom_Data;
  class Network;
  class Raven_Connect_Gate;
  class Raven_Connect_Output;
  class Raven_Disconnect_Gate;
  class Raven_Decrement_Output_Count;
  class Raven_Disconnect_Output;
  class Raven_Status_Empty;
  class Raven_Status_Nonempty;
  class Raven_Token_Insert;
  class Raven_Token_Remove;

  class Node : public Concurrency::Maester
  {
    Node(const Node &) = delete;
    Node & operator=(const Node &) = delete;

  public:
    typedef std::unordered_multiset<std::shared_ptr<Node>> Outputs;

    class Unlocked_Node_Data;
    class Locked_Node_Data_Const;
    class Locked_Node_Data;

  protected:
    ZENI_RETE_LINKAGE std::shared_ptr<const Node> shared_from_this() const;
    ZENI_RETE_LINKAGE std::shared_ptr<Node> shared_from_this();

    ZENI_RETE_LINKAGE Node(const int64_t height, const int64_t size, const int64_t token_size, const bool increment_output_count);

    ZENI_RETE_LINKAGE virtual void send_disconnect_from_parents(const std::shared_ptr<Network> network, const Locked_Node_Data &locked_node_data) = 0;

  public:
    class Unlocked_Node_Data {
      Unlocked_Node_Data(const Unlocked_Node_Data &) = delete;
      Unlocked_Node_Data & operator=(const Unlocked_Node_Data &) = delete;

      friend Locked_Node_Data_Const;
      friend Locked_Node_Data;

    public:
      Unlocked_Node_Data(const bool increment_output_count);

    private:
      int64_t m_output_count;
      Outputs m_outputs;
      Outputs m_antioutputs;
      Tokens m_output_tokens;
      Outputs m_gates;
      Outputs m_antigates;
    };

    class Locked_Node_Data_Const {
      Locked_Node_Data_Const(const Locked_Node_Data_Const &) = delete;
      Locked_Node_Data_Const & operator=(const Locked_Node_Data_Const &) = delete;

      friend Locked_Node_Data;

    public:
      ZENI_RETE_LINKAGE Locked_Node_Data_Const(const Node * node);

      ZENI_RETE_LINKAGE int64_t get_output_count() const;
      ZENI_RETE_LINKAGE const Outputs & get_outputs() const;
      ZENI_RETE_LINKAGE const Outputs & get_antioutputs() const;
      ZENI_RETE_LINKAGE const Tokens & get_output_tokens() const;
      ZENI_RETE_LINKAGE const Outputs & get_gates() const;
      ZENI_RETE_LINKAGE const Outputs & get_antigates() const;

    private:
      const Concurrency::Mutex::Lock m_lock;
      const std::shared_ptr<const Unlocked_Node_Data> m_data;
    };

    class Locked_Node_Data : public Locked_Node_Data_Const {
      Locked_Node_Data(const Locked_Node_Data &) = delete;
      Locked_Node_Data & operator=(const Locked_Node_Data &) = delete;

    public:
      ZENI_RETE_LINKAGE Locked_Node_Data(Node * node);

      ZENI_RETE_LINKAGE int64_t & modify_output_count();
      ZENI_RETE_LINKAGE Outputs & modify_outputs();
      ZENI_RETE_LINKAGE Outputs & modify_antioutputs();
      ZENI_RETE_LINKAGE Tokens & modify_output_tokens();
      ZENI_RETE_LINKAGE Outputs & modify_gates();
      ZENI_RETE_LINKAGE Outputs & modify_antigates();

    private:
      const std::shared_ptr<Unlocked_Node_Data> m_data;
    };

    ZENI_RETE_LINKAGE int64_t get_height() const;
    ZENI_RETE_LINKAGE int64_t get_size() const;
    ZENI_RETE_LINKAGE int64_t get_token_size() const;

    ZENI_RETE_LINKAGE virtual std::pair<std::shared_ptr<Node>, std::shared_ptr<Node>> get_inputs() = 0;

    /// Increment the output count. Only function that ought to result in a double mutex lock, when a parent Node calls increment_output_count on a child Node.
    ZENI_RETE_LINKAGE void increment_output_count();
    /// Find an existing equivalent to output and return it, or return the new output if no equivalent exists.
    ZENI_RETE_LINKAGE std::shared_ptr<Node> connect_gate(const std::shared_ptr<Network> network, const std::shared_ptr<Node> output, const bool immediate);
    /// Find an existing equivalent to output and return it, or return the new output if no equivalent exists.
    ZENI_RETE_LINKAGE std::shared_ptr<Node> connect_output(const std::shared_ptr<Network> network, const std::shared_ptr<Node> output, const bool immediate);

    ZENI_RETE_LINKAGE void receive(Concurrency::Job_Queue &job_queue, const std::shared_ptr<const Concurrency::Raven> raven) override;
    ZENI_RETE_LINKAGE void receive(const Raven_Connect_Gate &raven);
    ZENI_RETE_LINKAGE void receive(const Raven_Connect_Output &raven);
    ZENI_RETE_LINKAGE void receive(const Raven_Decrement_Output_Count &raven);
    ZENI_RETE_LINKAGE void receive(const Raven_Disconnect_Gate &raven);
    ZENI_RETE_LINKAGE void receive(const Raven_Disconnect_Output &raven);
    ZENI_RETE_LINKAGE virtual void receive(const Raven_Status_Empty &raven) = 0;
    ZENI_RETE_LINKAGE virtual void receive(const Raven_Status_Nonempty &raven) = 0;
    ZENI_RETE_LINKAGE virtual void receive(const Raven_Token_Insert &raven) = 0;
    ZENI_RETE_LINKAGE virtual void receive(const Raven_Token_Remove &raven) = 0;

    ZENI_RETE_LINKAGE virtual bool operator==(const Node &rhs) const = 0;

  private:
    const int64_t m_height;
    const int64_t m_size;
    const int64_t m_token_size;

    std::shared_ptr<Unlocked_Node_Data> m_unlocked_node_data;

  public:
    std::shared_ptr<Custom_Data> custom_data;
  };

}

#endif
