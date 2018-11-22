#include "Zeni/Rete/Node_Join.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Internal/Message_Connect_Join.hpp"
#include "Zeni/Rete/Internal/Message_Disconnect_Output.hpp"
#include "Zeni/Rete/Internal/Message_Token_Insert.hpp"
#include "Zeni/Rete/Internal/Message_Token_Remove.hpp"
#include "Zeni/Rete/Internal/Token_Beta.hpp"
#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Node_Key.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Join::Node_Join(const std::shared_ptr<const Node_Key> node_key_left, const std::shared_ptr<const Node_Key> node_key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const Variable_Bindings &variable_bindings)
    : Node_Binary(std::max(input_left->get_height(), input_right->get_height()) + 1, input_left->get_size() + input_right->get_size() + 1, input_left->get_token_size() + input_right->get_token_size(), hash_combine(hash_combine(std::hash<int>()(2), node_key_left->hash()), node_key_right->hash()), node_key_left, node_key_right, input_left, input_right),
    m_variable_bindings(variable_bindings)
  {
  }

  Node_Join::Node_Join(const std::shared_ptr<const Node_Key> node_key_left, const std::shared_ptr<const Node_Key> node_key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, Variable_Bindings &&variable_bindings)
    : Node_Binary(std::max(input_left->get_height(), input_right->get_height()) + 1, input_left->get_size() + input_right->get_size() + 1, input_left->get_token_size() + input_right->get_token_size(), hash_combine(hash_combine(std::hash<int>()(2), node_key_left->hash()), node_key_right->hash()), node_key_left, node_key_right, input_left, input_right),
    m_variable_bindings(std::move(variable_bindings))
  {
  }

  std::shared_ptr<Node_Join> Node_Join::Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> node_key_left, const std::shared_ptr<const Node_Key> node_key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const Variable_Bindings &variable_bindings) {
    class Friendly_Node_Join : public Node_Join {
    public:
      Friendly_Node_Join(const std::shared_ptr<const Node_Key> node_key_left, const std::shared_ptr<const Node_Key> node_key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const Variable_Bindings variable_bindings) : Node_Join(node_key_left, node_key_right, input_left, input_right, variable_bindings) {}
    };

    const auto created = std::shared_ptr<Friendly_Node_Join>(new Friendly_Node_Join(node_key_left, node_key_right, input_left, input_right, variable_bindings));

    return connect_created(network, job_queue, input_left, input_right, created);
  }

  std::shared_ptr<Node_Join> Node_Join::Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> node_key_left, const std::shared_ptr<const Node_Key> node_key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, Variable_Bindings &&variable_bindings) {
    class Friendly_Node_Join : public Node_Join {
    public:
      Friendly_Node_Join(const std::shared_ptr<const Node_Key> node_key_left, const std::shared_ptr<const Node_Key> node_key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, Variable_Bindings &&variable_bindings) : Node_Join(node_key_left, node_key_right, input_left, input_right, std::move(variable_bindings)) {}
    };

    const auto created = std::shared_ptr<Friendly_Node_Join>(new Friendly_Node_Join(node_key_left, node_key_right, input_left, input_right, std::move(variable_bindings)));

    return connect_created(network, job_queue, input_left, input_right, created);
  }

  std::shared_ptr<Node_Join> Node_Join::connect_created(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const std::shared_ptr<Node_Join> created) {
    const auto[result0, connected] = input_left->connect_new_or_existing_output(network, job_queue, created->get_key_left(), created);

    const auto connected_join = std::static_pointer_cast<Node_Join>(connected);

    if (result0 != Node_Trie::Result::First_Insertion) {
      assert(input_left == connected_join->get_input_left());
      assert(input_right == connected_join->get_input_right());
      input_left->send_disconnect_from_parents(network, job_queue);
    }

    if (input_left == input_right && *connected_join->get_key_left() == *connected_join->get_key_right())
      input_right->send_disconnect_from_parents(network, job_queue);
    else {
      const auto result1 = input_right->connect_existing_output(network, job_queue, connected_join->get_key_right(), connected);
      if (result1 != Node_Trie::Result::First_Insertion)
        input_right->send_disconnect_from_parents(network, job_queue);
    }

    return connected_join;
  }

  std::pair<Node::Node_Trie::Result, std::shared_ptr<Node>> Node_Join::connect_new_or_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) {
    const auto sft = shared_from_this();

    assert(dynamic_cast<const Node_Key_Null *>(key.get()));

    const auto[result, snapshot, value] = m_join_layer_trie.insert_ip_xp<JOIN_LAYER_OUTPUTS_UNLINKED, JOIN_LAYER_OUTPUTS>(child);

    if (result == Node_Trie::Result::First_Insertion)
      job_queue->give_one(std::make_shared<Message_Connect_Join>(sft, network, std::move(snapshot), key, value));

    return std::make_pair(result, value);
  }

  Node::Node_Trie::Result Node_Join::connect_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) {
    const auto sft = shared_from_this();

    assert(dynamic_cast<const Node_Key_Null *>(key.get()));

    const auto[result, snapshot, value] = m_join_layer_trie.insert_ip_xp<JOIN_LAYER_OUTPUTS, JOIN_LAYER_OUTPUTS_UNLINKED>(child);

    assert(value == child);

    //if (result == Node_Trie::Result::First_Insertion_IP)
    //  job_queue->give_one(std::make_shared<Message_Connect_Join>(sft, network, std::move(snapshot), key, value));

    return result;
  }

  Node::Node_Trie::Result Node_Join::link_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) {
    const auto sft = shared_from_this();

    assert(std::dynamic_pointer_cast<const Node_Key_Null>(key));

    const auto[result, snapshot, value] = m_join_layer_trie.move<JOIN_LAYER_OUTPUTS_UNLINKED, JOIN_LAYER_OUTPUTS>(child);

    if (result != Node_Trie::Result::Successful_Move)
      return result;

    insert_tokens(network, job_queue, key, child, snapshot);

    return result;
  }

  Node::Node_Trie::Result Node_Join::unlink_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) {
    const auto sft = shared_from_this();

    assert(std::dynamic_pointer_cast<const Node_Key_Null>(key));

    const auto[result, snapshot, value] = m_join_layer_trie.move<JOIN_LAYER_OUTPUTS, JOIN_LAYER_OUTPUTS_UNLINKED>(child);

    if (result != Node_Trie::Result::Successful_Move)
      return result;

    remove_tokens(network, job_queue, key, child, snapshot);

    return result;
  }

  bool Node_Join::has_tokens([[maybe_unused]] const std::shared_ptr<const Node_Key> key) const {
    assert(dynamic_cast<const Node_Key_Null *>(key.get()));

    const auto tokens = m_join_layer_trie.snapshot<JOIN_LAYER_OUTPUT_TOKENS>();
    return !tokens.size_zero();
  }

  void Node_Join::receive(const Message_Token_Insert &message) {
    const auto sft = shared_from_this();
    const auto &job_queue = message.get_Job_Queue();

    if (message.parent == get_input_left() && *message.key == *get_key_left()) {
      const auto symbols = bind_variables_left(message.token);

      const auto[result, snapshot, value] = m_join_layer_trie.insert_2<JOIN_LAYER_SYMBOLS, JOIN_LAYER_SYMBOLS_TOKENS_LEFT>(symbols, message.token);
      if (result == Token_Trie::Result::First_Insertion) {
        get_input_right()->link_output(message.network, message.get_Job_Queue(), get_key_right(), sft); ///< Temporary bludgeon

        for (auto token_right : snapshot.lookup_snapshot<JOIN_LAYER_SYMBOLS, JOIN_LAYER_SYMBOLS_TOKENS_RIGHT>(symbols)) {
          const auto output_token = std::make_shared<Token_Beta>(value, token_right);

          const auto[oresult, osnapshot, ovalue] = m_join_layer_trie.insert<JOIN_LAYER_OUTPUT_TOKENS>(output_token);
          if (oresult != Output_Token_Trie::Result::First_Insertion)
            continue;

          for (auto &output : osnapshot.snapshot<JOIN_LAYER_OUTPUTS>())
            job_queue->give_one(std::make_shared<Message_Token_Insert>(output, message.network, sft, Node_Key_Null::Create(), output_token));
        }
      }
    }

    if (message.parent == get_input_right() && *message.key == *get_key_right()) {
      const auto symbols = bind_variables_right(message.token);

      const auto[result, snapshot, value] = m_join_layer_trie.insert_2<JOIN_LAYER_SYMBOLS, JOIN_LAYER_SYMBOLS_TOKENS_RIGHT>(symbols, message.token);
      if (result == Token_Trie::Result::First_Insertion) {
        for (auto token_left : snapshot.lookup_snapshot<JOIN_LAYER_SYMBOLS, JOIN_LAYER_SYMBOLS_TOKENS_LEFT>(symbols)) {
          const auto output_token = std::make_shared<Token_Beta>(token_left, value);

          const auto[oresult, osnapshot, ovalue] = m_join_layer_trie.insert<JOIN_LAYER_OUTPUT_TOKENS>(output_token);
          if (oresult != Output_Token_Trie::Result::First_Insertion)
            continue;

          for (auto &output : osnapshot.snapshot<JOIN_LAYER_OUTPUTS>())
            job_queue->give_one(std::make_shared<Message_Token_Insert>(output, message.network, sft, Node_Key_Null::Create(), output_token));
        }
      }
    }
  }

  void Node_Join::receive(const Message_Token_Remove &message) {
    const auto sft = shared_from_this();
    const auto &job_queue = message.get_Job_Queue();

    if (message.parent == get_input_left() && *message.key == *get_key_left()) {
      const auto symbols = bind_variables_left(message.token);

      const auto[result, snapshot, value] = m_join_layer_trie.erase_2<JOIN_LAYER_SYMBOLS, JOIN_LAYER_SYMBOLS_TOKENS_LEFT>(symbols, message.token);
      if (result == Token_Trie::Result::Last_Removal) {
        for (auto token_right : snapshot.lookup_snapshot<JOIN_LAYER_SYMBOLS, JOIN_LAYER_SYMBOLS_TOKENS_RIGHT>(symbols)) {
          const auto output_token = std::make_shared<Token_Beta>(value, token_right);

          const auto[oresult, osnapshot, ovalue] = m_join_layer_trie.erase<JOIN_LAYER_OUTPUT_TOKENS>(output_token);
          if (oresult != Output_Token_Trie::Result::Last_Removal)
            continue;

          for (auto &output : osnapshot.snapshot<JOIN_LAYER_OUTPUTS>())
            job_queue->give_one(std::make_shared<Message_Token_Remove>(output, message.network, sft, Node_Key_Null::Create(), output_token));
        }
      }
    }

    if (message.parent == get_input_right() && *message.key == *get_key_right()) {
      const auto symbols = bind_variables_right(message.token);

      const auto[result, snapshot, value] = m_join_layer_trie.erase_2<JOIN_LAYER_SYMBOLS, JOIN_LAYER_SYMBOLS_TOKENS_RIGHT>(symbols, message.token);
      if (result == Token_Trie::Result::Last_Removal) {
        for (auto token_left : snapshot.lookup_snapshot<JOIN_LAYER_SYMBOLS, JOIN_LAYER_SYMBOLS_TOKENS_LEFT>(symbols)) {
          const auto output_token = std::make_shared<Token_Beta>(token_left, value);

          const auto[oresult, osnapshot, ovalue] = m_join_layer_trie.erase<JOIN_LAYER_OUTPUT_TOKENS>(output_token);
          if (oresult != Output_Token_Trie::Result::Last_Removal)
            continue;

          for (auto &output : osnapshot.snapshot<JOIN_LAYER_OUTPUTS>())
            job_queue->give_one(std::make_shared<Message_Token_Remove>(output, message.network, sft, Node_Key_Null::Create(), output_token));
        }
      }
    }
  }

  void Node_Join::receive(const Message_Connect_Join &message) {
    insert_tokens(message.network, message.get_Job_Queue(), message.key, message.child, message.snapshot);
  }

  void Node_Join::receive(const Message_Disconnect_Output &message) {
    assert(std::dynamic_pointer_cast<const Node_Key_Null>(message.key));

    const auto[result, snapshot, value] = m_join_layer_trie.erase_ip_xp<JOIN_LAYER_OUTPUTS, JOIN_LAYER_OUTPUTS_UNLINKED>(message.child);

    assert(result != Node_Trie::Result::Failed_Removal);

    if (result != Node_Trie::Result::Last_Removal_IP && result != Node_Trie::Result::Last_Removal)
      return;

    send_disconnect_from_parents(message.network, message.get_Job_Queue());

    if (result != Node_Trie::Result::Last_Removal_IP)
      return;

    remove_tokens(message.network, message.get_Job_Queue(), message.key, message.child, snapshot);
  }

  void Node_Join::insert_tokens(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child, const Join_Layer_Snapshot snapshot) {
    const auto sft = shared_from_this();

    assert(std::dynamic_pointer_cast<const Node_Key_Null>(key));

    for (const auto &token : snapshot.snapshot<JOIN_LAYER_OUTPUT_TOKENS>())
      job_queue->give_one(std::make_shared<Message_Token_Insert>(child, network, sft, Node_Key_Null::Create(), token));
  }

  void Node_Join::remove_tokens(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child, const Join_Layer_Snapshot snapshot) {
    const auto sft = shared_from_this();

    for (const auto &token : snapshot.snapshot<JOIN_LAYER_OUTPUT_TOKENS>())
      job_queue->give_one(std::make_shared<Message_Token_Remove>(child, network, sft, Node_Key_Null::Create(), token));
  }

  std::shared_ptr<Symbols> Node_Join::bind_variables_left(const std::shared_ptr<const Token> token_left) const {
    auto symbols = std::make_shared<Symbols>();
    symbols->reserve(m_variable_bindings.size());
    for (auto variable_binding : m_variable_bindings)
      symbols->emplace_back((*token_left)[variable_binding.first]);
    return symbols;
  }

  std::shared_ptr<Symbols> Node_Join::bind_variables_right(const std::shared_ptr<const Token> token_right) const {
    auto symbols = std::make_shared<Symbols>();
    symbols->reserve(m_variable_bindings.size());
    for (auto variable_binding : m_variable_bindings)
      symbols->emplace_back((*token_right)[variable_binding.second]);
    return symbols;
  }

  bool Node_Join::operator==(const Node &rhs) const {
    //return this == &rhs;

    if (auto rhs_join = dynamic_cast<const Node_Join *>(&rhs)) {
      return *get_key_left() == *rhs_join->get_key_left() && *get_key_right() == *rhs_join->get_key_right()
        && get_input_left() == rhs_join->get_input_left() && get_input_right() == rhs_join->get_input_right();
    }

    return false;
  }

}
