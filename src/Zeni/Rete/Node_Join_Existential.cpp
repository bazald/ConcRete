#include "Zeni/Rete/Node_Join_Existential.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Internal/Message_Connect_Join.hpp"
#include "Zeni/Rete/Internal/Message_Disconnect_Output.hpp"
#include "Zeni/Rete/Internal/Message_Relink_Output.hpp"
#include "Zeni/Rete/Internal/Message_Token_Insert.hpp"
#include "Zeni/Rete/Internal/Message_Token_Remove.hpp"
#include "Zeni/Rete/Internal/Token_Beta.hpp"
#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Node_Key.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Join_Existential::Node_Join_Existential(const std::shared_ptr<const Node_Key> node_key_left, const std::shared_ptr<const Node_Key> node_key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const Variable_Bindings &variable_bindings)
    : Node_Binary(std::max(input_left->get_height(), input_right->get_height()) + 1, input_left->get_size() + input_right->get_size() + 1, input_left->get_token_size() + input_right->get_token_size(), hash_combine(hash_combine(std::hash<int>()(2), node_key_left->hash()), node_key_right->hash()), node_key_left, node_key_right, input_left, input_right, variable_bindings)
  {
  }

  Node_Join_Existential::Node_Join_Existential(const std::shared_ptr<const Node_Key> node_key_left, const std::shared_ptr<const Node_Key> node_key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, Variable_Bindings &&variable_bindings)
    : Node_Binary(std::max(input_left->get_height(), input_right->get_height()) + 1, input_left->get_size() + input_right->get_size() + 1, input_left->get_token_size() + input_right->get_token_size(), hash_combine(hash_combine(std::hash<int>()(2), node_key_left->hash()), node_key_right->hash()), node_key_left, node_key_right, input_left, input_right, std::move(variable_bindings))
  {
  }

  std::shared_ptr<Node_Join_Existential> Node_Join_Existential::Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> node_key_left, const std::shared_ptr<const Node_Key> node_key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const Variable_Bindings &variable_bindings) {
    class Friendly_Node_Join_Existential : public Node_Join_Existential {
    public:
      Friendly_Node_Join_Existential(const std::shared_ptr<const Node_Key> node_key_left, const std::shared_ptr<const Node_Key> node_key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const Variable_Bindings variable_bindings) : Node_Join_Existential(node_key_left, node_key_right, input_left, input_right, variable_bindings) {}
    };

    const auto created = std::shared_ptr<Friendly_Node_Join_Existential>(new Friendly_Node_Join_Existential(node_key_left, node_key_right, input_left, input_right, variable_bindings));

    return std::dynamic_pointer_cast<Node_Join_Existential>(connect_created(network, job_queue, input_left, input_right, created));
  }

  std::shared_ptr<Node_Join_Existential> Node_Join_Existential::Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> node_key_left, const std::shared_ptr<const Node_Key> node_key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, Variable_Bindings &&variable_bindings) {
    class Friendly_Node_Join_Existential : public Node_Join_Existential {
    public:
      Friendly_Node_Join_Existential(const std::shared_ptr<const Node_Key> node_key_left, const std::shared_ptr<const Node_Key> node_key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, Variable_Bindings &&variable_bindings) : Node_Join_Existential(node_key_left, node_key_right, input_left, input_right, std::move(variable_bindings)) {}
    };

    const auto created = std::shared_ptr<Friendly_Node_Join_Existential>(new Friendly_Node_Join_Existential(node_key_left, node_key_right, input_left, input_right, std::move(variable_bindings)));

    return std::dynamic_pointer_cast<Node_Join_Existential>(connect_created(network, job_queue, input_left, input_right, created));
  }

  void Node_Join_Existential::receive(const Message_Token_Insert &message) {
    const auto sft = shared_from_this();
    const auto &job_queue = message.get_Job_Queue();

    if (message.parent == get_input_left() && message.key->is_contained_by(*get_key_left())) {
      const auto symbols = bind_variables_left(message.token);

      const auto[result, snapshot, value] = m_join_layer_trie.insert_2<JOIN_LAYER_SYMBOLS, JOIN_LAYER_SYMBOLS_TOKENS_LEFT>(symbols, message.token);
      if (result == Token_Trie::Result::First_Insertion) {
        if (!m_left_eq_right) {
          Concurrency::Intrusive_Shared_Ptr<Link_Data>::Lock expected = m_link_data.load(std::memory_order_acquire);
          for (;;) {
            Concurrency::Intrusive_Shared_Ptr<Link_Data>::Lock desired =
              new Link_Data(expected->tokens_left + 1, expected->tokens_right, expected->tokens_left == 0 ? Link_Status::BOTH_LINKED : expected->link_status);
            if (m_link_data.compare_exchange_strong(expected, desired, std::memory_order_release, std::memory_order_acquire)) {
              if (expected->link_status != desired->link_status) {
                if (const auto multisym = std::dynamic_pointer_cast<const Node_Key_Multisym>(get_key_right())) {
                  for (const auto sym : multisym->symbols)
                    job_queue->give_one(std::make_shared<Message_Relink_Output>(get_input_right(), message.network, sym, sft));
                }
                else
                  job_queue->give_one(std::make_shared<Message_Relink_Output>(get_input_right(), message.network, get_key_right(), sft));
              }
              break;
            }
          }
        }

        const auto tokens_right = snapshot.lookup_snapshot<JOIN_LAYER_SYMBOLS, JOIN_LAYER_SYMBOLS_TOKENS_RIGHT>(symbols);
        if (tokens_right.cbegin() != tokens_right.cend()) {
          const auto[oresult, osnapshot, ovalue] = m_join_layer_trie.insert<JOIN_LAYER_OUTPUT_TOKENS>(message.token);
          if (oresult == Output_Token_Trie::Result::First_Insertion) {
            for (auto &output : osnapshot.snapshot<JOIN_LAYER_OUTPUTS>())
              job_queue->give_one(std::make_shared<Message_Token_Insert>(output, message.network, sft, Node_Key_Null::Create(), message.token));
          }
        }
      }
    }

    if (message.parent == get_input_right() && message.key->is_contained_by(*get_key_right())) {
      const auto symbols = bind_variables_right(message.token);

      const auto[result, snapshot, value] = m_join_layer_trie.insert_2<JOIN_LAYER_SYMBOLS, JOIN_LAYER_SYMBOLS_TOKENS_RIGHT>(symbols, message.token);
      if (result == Token_Trie::Result::First_Insertion) {
        if (!m_left_eq_right) {
          Concurrency::Intrusive_Shared_Ptr<Link_Data>::Lock expected = m_link_data.load(std::memory_order_acquire);
          for (;;) {
            Concurrency::Intrusive_Shared_Ptr<Link_Data>::Lock desired =
              new Link_Data(expected->tokens_left, expected->tokens_right + 1, expected->tokens_right == 0 ? Link_Status::BOTH_LINKED : expected->link_status);
            if (m_link_data.compare_exchange_strong(expected, desired, std::memory_order_release, std::memory_order_acquire)) {
              if (expected->link_status != desired->link_status) {
                if (const auto multisym = std::dynamic_pointer_cast<const Node_Key_Multisym>(get_key_left())) {
                  for (const auto sym : multisym->symbols)
                    job_queue->give_one(std::make_shared<Message_Relink_Output>(get_input_left(), message.network, sym, sft));
                }
                else
                  job_queue->give_one(std::make_shared<Message_Relink_Output>(get_input_left(), message.network, get_key_left(), sft));
              }
              break;
            }
          }
        }

        const auto tokens_right = snapshot.lookup_snapshot<JOIN_LAYER_SYMBOLS, JOIN_LAYER_SYMBOLS_TOKENS_RIGHT>(symbols);
        if (++tokens_right.cbegin() == tokens_right.cend()) {
          for (auto token_left : snapshot.lookup_snapshot<JOIN_LAYER_SYMBOLS, JOIN_LAYER_SYMBOLS_TOKENS_LEFT>(symbols)) {
            const auto[oresult, osnapshot, ovalue] = m_join_layer_trie.insert<JOIN_LAYER_OUTPUT_TOKENS>(token_left);
            if (oresult != Output_Token_Trie::Result::First_Insertion)
              continue;

            for (auto &output : osnapshot.snapshot<JOIN_LAYER_OUTPUTS>())
              job_queue->give_one(std::make_shared<Message_Token_Insert>(output, message.network, sft, Node_Key_Null::Create(), token_left));
          }
        }
      }
    }
  }

  void Node_Join_Existential::receive(const Message_Token_Remove &message) {
    const auto sft = shared_from_this();
    const auto &job_queue = message.get_Job_Queue();

    if (message.parent == get_input_left() && message.key->is_contained_by(*get_key_left())) {
      const auto symbols = bind_variables_left(message.token);

      const auto[result, snapshot, value] = m_join_layer_trie.erase_2<JOIN_LAYER_SYMBOLS, JOIN_LAYER_SYMBOLS_TOKENS_LEFT>(symbols, message.token);
      if (result == Token_Trie::Result::Last_Removal) {
        if (!m_left_eq_right) {
          Concurrency::Intrusive_Shared_Ptr<Link_Data>::Lock expected = m_link_data.load(std::memory_order_acquire);
          for (;;) {
            Concurrency::Intrusive_Shared_Ptr<Link_Data>::Lock desired =
              new Link_Data(expected->tokens_left - 1, expected->tokens_right, expected->tokens_left == 1 && expected->link_status == Link_Status::BOTH_LINKED ? Link_Status::RIGHT_UNLINKED : expected->link_status);
            if (m_link_data.compare_exchange_strong(expected, desired, std::memory_order_release, std::memory_order_acquire)) {
              if (expected->link_status != desired->link_status) {
                if (const auto multisym = std::dynamic_pointer_cast<const Node_Key_Multisym>(get_key_right())) {
                  for (const auto sym : multisym->symbols)
                    job_queue->give_one(std::make_shared<Message_Relink_Output>(get_input_right(), message.network, sym, sft));
                }
                else
                  job_queue->give_one(std::make_shared<Message_Relink_Output>(get_input_right(), message.network, get_key_right(), sft));
              }
              break;
            }
          }
        }

        const auto tokens_right = snapshot.lookup_snapshot<JOIN_LAYER_SYMBOLS, JOIN_LAYER_SYMBOLS_TOKENS_RIGHT>(symbols);
        if (tokens_right.cbegin() != tokens_right.cend()) {
          const auto[oresult, osnapshot, ovalue] = m_join_layer_trie.erase<JOIN_LAYER_OUTPUT_TOKENS>(message.token);
          if (oresult == Output_Token_Trie::Result::Last_Removal) {
            for (auto &output : osnapshot.snapshot<JOIN_LAYER_OUTPUTS>())
              job_queue->give_one(std::make_shared<Message_Token_Remove>(output, message.network, sft, Node_Key_Null::Create(), message.token));
          }
        }
      }
    }

    if (message.parent == get_input_right() && message.key->is_contained_by(*get_key_right())) {
      const auto symbols = bind_variables_right(message.token);

      const auto[result, snapshot, value] = m_join_layer_trie.erase_2<JOIN_LAYER_SYMBOLS, JOIN_LAYER_SYMBOLS_TOKENS_RIGHT>(symbols, message.token);
      if (result == Token_Trie::Result::Last_Removal) {
        if (!m_left_eq_right) {
          Concurrency::Intrusive_Shared_Ptr<Link_Data>::Lock expected = m_link_data.load(std::memory_order_acquire);
          for (;;) {
            Concurrency::Intrusive_Shared_Ptr<Link_Data>::Lock desired =
              new Link_Data(expected->tokens_left, expected->tokens_right - 1, expected->tokens_right == 1 && expected->link_status == Link_Status::BOTH_LINKED ? Link_Status::LEFT_UNLINKED : expected->link_status);
            if (m_link_data.compare_exchange_strong(expected, desired, std::memory_order_release, std::memory_order_acquire)) {
              if (expected->link_status != desired->link_status) {
                if (const auto multisym = std::dynamic_pointer_cast<const Node_Key_Multisym>(get_key_left())) {
                  for (const auto sym : multisym->symbols)
                    job_queue->give_one(std::make_shared<Message_Relink_Output>(get_input_left(), message.network, sym, sft));
                }
                else
                  job_queue->give_one(std::make_shared<Message_Relink_Output>(get_input_left(), message.network, get_key_left(), sft));
              }
              break;
            }
          }
        }

        const auto tokens_right = snapshot.lookup_snapshot<JOIN_LAYER_SYMBOLS, JOIN_LAYER_SYMBOLS_TOKENS_RIGHT>(symbols);
        if (tokens_right.cbegin() == tokens_right.cend()) {
          for (auto token_left : snapshot.lookup_snapshot<JOIN_LAYER_SYMBOLS, JOIN_LAYER_SYMBOLS_TOKENS_LEFT>(symbols)) {
            const auto[oresult, osnapshot, ovalue] = m_join_layer_trie.erase<JOIN_LAYER_OUTPUT_TOKENS>(token_left);
            if (oresult != Output_Token_Trie::Result::Last_Removal)
              continue;

            for (auto &output : osnapshot.snapshot<JOIN_LAYER_OUTPUTS>())
              job_queue->give_one(std::make_shared<Message_Token_Remove>(output, message.network, sft, Node_Key_Null::Create(), token_left));
          }
        }
      }
    }
  }

  bool Node_Join_Existential::operator==(const Node &rhs) const {
    //return this == &rhs;

    if (auto rhs_join = dynamic_cast<const Node_Join_Existential *>(&rhs)) {
      return *get_key_left() == *rhs_join->get_key_left() && *get_key_right() == *rhs_join->get_key_right()
        && get_input_left() == rhs_join->get_input_left() && get_input_right() == rhs_join->get_input_right();
    }

    return false;
  }

}
