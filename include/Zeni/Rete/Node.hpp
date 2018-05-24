#ifndef ZENI_RETE_NODE_H
#define ZENI_RETE_NODE_H

#include "Custom_Data.hpp"
#include "Pseudonode.hpp"
#include "Token.hpp"

namespace Zeni {

  namespace Rete {

    class Custom_Data;
    class Network;
    class Raven_Disconnect_Output;
    class Raven_Token_Insert;
    class Raven_Token_Remove;

    class Node : public Pseudonode
    {
      Node(const Node &) = delete;
      Node & operator=(const Node &) = delete;

    public:
      typedef std::unordered_set<std::shared_ptr<Node>> Outputs;

      class Unlocked_Node_Data;
      class Locked_Node_Data_Const;
      class Locked_Node_Data;

    protected:
      ZENI_RETE_LINKAGE std::shared_ptr<const Node> shared_from_this() const;
      ZENI_RETE_LINKAGE std::shared_ptr<Node> shared_from_this();

      ZENI_RETE_LINKAGE Node(const int64_t &height, const int64_t &size, const int64_t &token_size);

      /// Assumes the lock is held
      ZENI_RETE_LINKAGE virtual void send_disconnect_from_parents(const std::shared_ptr<Network> &network, class Locked_Node_Data &locked_node_data) = 0;

    public:
      class Unlocked_Node_Data {
        Unlocked_Node_Data(const Unlocked_Node_Data &) = delete;
        Unlocked_Node_Data & operator=(const Unlocked_Node_Data &) = delete;

        friend Locked_Node_Data_Const;
        friend Locked_Node_Data;

      public:
        Unlocked_Node_Data() {}

      private:
        int64_t m_output_count = 1;
        Outputs m_outputs;
        Tokens m_output_tokens;
      };

      class Locked_Node_Data_Const {
        Locked_Node_Data_Const(const Locked_Node_Data_Const &) = delete;
        Locked_Node_Data_Const & operator=(const Locked_Node_Data_Const &) = delete;

        friend Locked_Node_Data;

      public:
        ZENI_RETE_LINKAGE Locked_Node_Data_Const(const Node * const &node);

        ZENI_RETE_LINKAGE int64_t get_output_count() const;
        ZENI_RETE_LINKAGE const Outputs & get_outputs() const;
        ZENI_RETE_LINKAGE const Tokens & get_output_tokens() const;

      private:
        Concurrency::Mutex::Lock m_lock;
        std::shared_ptr<const Unlocked_Node_Data> m_data;
      };

      class Locked_Node_Data : public Locked_Node_Data_Const {
        Locked_Node_Data(const Locked_Node_Data &) = delete;
        Locked_Node_Data & operator=(const Locked_Node_Data &) = delete;

      public:
        ZENI_RETE_LINKAGE Locked_Node_Data(Node * const &node);

        ZENI_RETE_LINKAGE int64_t & modify_output_count();
        ZENI_RETE_LINKAGE Outputs & modify_outputs();
        ZENI_RETE_LINKAGE Tokens & modify_output_tokens();

      private:
        std::shared_ptr<Unlocked_Node_Data> m_data;
      };

      ZENI_RETE_LINKAGE int64_t get_height() const;
      ZENI_RETE_LINKAGE int64_t get_size() const;
      ZENI_RETE_LINKAGE int64_t get_token_size() const;

      /// Increment the output count
      ZENI_RETE_LINKAGE void increment_output_count();
      /// Does *not* increment the output count, under the assumption that gaining access to this object has already resulted in its increment
      ZENI_RETE_LINKAGE void connect_output(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output);
      /// Decrements the output count, potentially resulting in cascading disconnects
      ZENI_RETE_LINKAGE void disconnect_output(const std::shared_ptr<Network> &network, const std::shared_ptr<const Node> &output) override;

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

}

#endif
