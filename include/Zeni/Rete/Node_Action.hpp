#ifndef ZENI_RETE_NODE_ACTION_HPP
#define ZENI_RETE_NODE_ACTION_HPP

#include "Internal/Node_Unary.hpp"

#include <functional>

namespace Zeni::Rete {

  class Node_Action : public Node_Unary {
    Node_Action(const Node_Action &) = delete;
    Node_Action & operator=(const Node_Action &) = delete;

  public:
    class Data {
      Data(const Data &) = delete;
      Data & operator=(const Data &) = delete;

    public:
      class Hash {
      public:
        size_t operator()(const Data &data) const { return std::hash<Token>()(*data.input_token); }
        size_t operator()(const std::shared_ptr<const Data> data) const { return std::hash<Token>()(*data->input_token); }
      };

      Data(const std::shared_ptr<const Variable_Indices> variable_indices_, const std::shared_ptr<const Token> input_token_) : variable_indices(variable_indices_), input_token(input_token_) {}

      bool operator==(const Data &rhs) const { return *input_token == *rhs.input_token; }

      /// Key

      const std::shared_ptr<const Token> input_token;

      /// Data

      enum State : uint8_t {STATE_UNFIRED = 0, STATE_INTERMEDIATE = 1, STATE_MUST_RETRACT = 2};

      struct ZENI_CONCURRENCY_CACHE_ALIGN {
        mutable std::atomic_uint8_t state = STATE_UNFIRED;
      };

      mutable std::shared_ptr<const Symbol> result;
      mutable std::shared_ptr<const Variable_Indices> variable_indices;
      mutable std::shared_ptr<const Token> token = input_token;
    };

    class Action : public std::enable_shared_from_this<Action> {
      Action(const Action &) = delete;
      Action & operator=(const Action &) = delete;

    protected:
      Action() {}

    public:
      virtual ~Action() {}

      virtual void operator()(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node_Action> rete_action, const std::shared_ptr<const Data> action_data) const = 0;
    };

    class Null_Action : public Action {
      Null_Action(const Null_Action &) = delete;
      Null_Action & operator=(const Null_Action &) = delete;

      Null_Action() {}

    public:
      static std::shared_ptr<const Null_Action> Create();

      void operator()(const std::shared_ptr<Network>, const std::shared_ptr<Concurrency::Job_Queue>, const std::shared_ptr<Node_Action>, const std::shared_ptr<const Data>) const override {}
    };

    struct Compare_By_Name_Eq {
      bool operator()(const std::shared_ptr<Node_Action> &lhs, const std::shared_ptr<Node_Action> &rhs) const noexcept {
        return lhs->get_name() == rhs->get_name();
      }

      bool operator()(const Node_Action &lhs, const Node_Action &rhs) const noexcept {
        return lhs.get_name() == rhs.get_name();
      }

      bool operator()(const std::string_view &lhs, const std::string_view &rhs) const noexcept {
        return lhs == rhs;
      }

      bool operator()(const std::shared_ptr<Node_Action> &lhs, const std::string_view &rhs) const noexcept {
        return lhs->get_name() == rhs;
      }

      bool operator()(const std::string_view &lhs, const std::shared_ptr<Node_Action> &rhs) const noexcept {
        return lhs == rhs->get_name();
      }

      bool operator()(const Node_Action &lhs, const std::string_view &rhs) const noexcept {
        return lhs.get_name() == rhs;
      }

      bool operator()(const std::string_view &lhs, const Node_Action &rhs) const noexcept {
        return lhs == rhs.get_name();
      }
    };

    struct Hash_By_Name {
      size_t operator()(const std::shared_ptr<Node_Action> &rete_action) const noexcept {
        return std::hash<std::string_view>()(rete_action->get_name());
      }

      size_t operator()(const Node_Action &rete_action) const noexcept {
        return std::hash<std::string_view>()(rete_action.get_name());
      }

      size_t operator()(const std::string_view &name) const noexcept {
        return std::hash<std::string_view>()(name);
      }
    };

  private:
    Node_Action(const std::string_view name, const std::shared_ptr<const Node_Key> node_key, const std::shared_ptr<Node> input, const std::shared_ptr<const Variable_Indices> variable_indices, const std::shared_ptr<const Action> action, const std::shared_ptr<const Action> retraction);

  public:
    Node_Action(const std::string_view name);

    ZENI_RETE_LINKAGE static std::shared_ptr<Node_Action> Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::string_view name, const bool user_action, const std::shared_ptr<const Node_Key> node_key, const std::shared_ptr<Node> input, const std::shared_ptr<const Variable_Indices> variable_indices, const std::shared_ptr<const Action> action, const std::shared_ptr<const Action> retraction = Null_Action::Create());

    ZENI_RETE_LINKAGE ~Node_Action();

    ZENI_RETE_LINKAGE void Destroy(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue);

    ZENI_RETE_LINKAGE std::string get_name() const;
    ZENI_RETE_LINKAGE std::shared_ptr<const Variable_Indices> get_variable_indices() const;

    ZENI_RETE_LINKAGE std::pair<Node_Trie::Result, std::shared_ptr<Node>> connect_new_or_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) override;
    ZENI_RETE_LINKAGE Node_Trie::Result connect_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) override;

    ZENI_RETE_LINKAGE void receive(const Message_Token_Insert &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Token_Remove &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Disconnect_Output &message) override;

    ZENI_RETE_LINKAGE bool operator==(const Node &) const override;

  private:
    typedef Concurrency::Antiable_Hash_Trie<std::shared_ptr<const Data>, Data::Hash, compare_deref_eq> Action_Trie;

    std::shared_ptr<const Variable_Indices> m_variable_indices;
    const std::string m_name;
    std::shared_ptr<const Action> m_action;
    std::shared_ptr<const Action> m_retraction;

    Action_Trie m_action_trie;
  };

}

#endif
