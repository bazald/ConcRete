#include "Zeni/Rete/Internal/Node_Binary.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Internal/Job_Sequential_Messages.hpp"
#include "Zeni/Rete/Internal/Message_Connect_Join.hpp"
#include "Zeni/Rete/Internal/Message_Disconnect_Output.hpp"
#include "Zeni/Rete/Internal/Message_Relink_Output.hpp"
#include "Zeni/Rete/Internal/Message_Token_Insert.hpp"
#include "Zeni/Rete/Internal/Message_Token_Remove.hpp"
#include "Zeni/Rete/Network.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Binary::Node_Binary(const int64_t height, const int64_t size, const int64_t token_size, const size_t hash, const std::shared_ptr<const Node_Key> key_left, const std::shared_ptr<const Node_Key> key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const Variable_Bindings &variable_bindings)
    : Node(height, size, token_size, hash),
    m_key_left(key_left),
    m_key_right(key_right),
    m_input_left(input_left),
    m_input_right(input_right),
    m_left_eq_right(input_left == input_right && *key_left == *key_right),
    m_variable_bindings(variable_bindings)
  {
  }

  Node_Binary::Node_Binary(const int64_t height, const int64_t size, const int64_t token_size, const size_t hash, const std::shared_ptr<const Node_Key> key_left, const std::shared_ptr<const Node_Key> key_right, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, Variable_Bindings &&variable_bindings)
    : Node(height, size, token_size, hash),
    m_key_left(key_left),
    m_key_right(key_right),
    m_input_left(input_left),
    m_input_right(input_right),
    m_left_eq_right(input_left == input_right && *key_left == *key_right),
    m_variable_bindings(std::move(variable_bindings))
  {
  }

  void Node_Binary::connect_to_parents_again(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) {
    const auto sft = shared_from_this();

    if (const auto multisym_left = std::dynamic_pointer_cast<const Node_Key_Multisym>(m_key_left)) {
      for (auto sym_left : multisym_left->symbols)
        m_input_left->connect_existing_output(network, job_queue, sym_left, sft, false);
    }
    else
      m_input_left->connect_existing_output(network, job_queue, m_key_left, sft, false);

    if (!m_left_eq_right) {
      if (const auto multisym_right = std::dynamic_pointer_cast<const Node_Key_Multisym>(m_key_right)) {
        for (auto sym_right : multisym_right->symbols)
          m_input_right->connect_existing_output(network, job_queue, sym_right, sft, true);
      }
      else
        m_input_right->connect_existing_output(network, job_queue, m_key_right, sft, true);
    }
  }

  void Node_Binary::send_disconnect_from_parents(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) {
    const auto sft = shared_from_this();

    const auto multisym_left = std::dynamic_pointer_cast<const Node_Key_Multisym>(m_key_left);
    const auto multisym_right = std::dynamic_pointer_cast<const Node_Key_Multisym>(m_key_right);
    
    const int64_t total = int64_t((multisym_left ? multisym_left->symbols.size() : 1) + (m_left_eq_right ? 0 : multisym_right ? multisym_right->symbols.size() : 1));

    if (total == 1)
      job_queue->give_one(std::make_shared<Message_Disconnect_Output>(m_input_left, network, m_key_left, sft));
    else if (total == 2) {
      if (multisym_left) {
        auto mt = multisym_left->symbols.cbegin();
        const auto key_left = *mt++;
        const auto key_right = *mt;
        job_queue->give_one(std::make_shared<Job_Sequential_Messages>(network,
          std::make_shared<Message_Disconnect_Output>(m_input_right, network, key_right, sft),
          std::make_shared<Message_Disconnect_Output>(m_input_left, network, key_left, sft)));
      }
      else {
        job_queue->give_one(std::make_shared<Job_Sequential_Messages>(network,
          std::make_shared<Message_Disconnect_Output>(m_input_right, network, m_key_right, sft),
          std::make_shared<Message_Disconnect_Output>(m_input_left, network, m_key_left, sft)));
      }
    }
    else {
      auto mt = multisym_left ? multisym_left->symbols.cbegin() : Node_Key_Multisym::Node_Key_Symbol_Trie::const_iterator();
      const auto last_message = std::make_shared<Message_Disconnect_Output>(m_input_left, network, multisym_left ? *mt++ : m_key_left, shared_from_this());
      const auto counter = std::make_shared<std::atomic_int64_t>(total - 1);

      if (multisym_left) {
        for (const auto mend = multisym_left->symbols.cend(); mt != mend; ++mt) {
          job_queue->give_one(std::make_shared<Job_Sequential_Messages_Countdown>(network,
            std::make_shared<Message_Disconnect_Output>(m_input_left, network, *mt, shared_from_this()),
            last_message,
            counter));
        }
      }

      if (!m_left_eq_right) {
        if (multisym_right) {
          for (auto sym_right : multisym_right->symbols) {
            job_queue->give_one(std::make_shared<Job_Sequential_Messages_Countdown>(network,
              std::make_shared<Message_Disconnect_Output>(m_input_right, network, sym_right, shared_from_this()),
              last_message,
              counter));
          }
        }
        else {
          job_queue->give_one(std::make_shared<Job_Sequential_Messages_Countdown>(network,
            std::make_shared<Message_Disconnect_Output>(m_input_right, network, m_key_right, shared_from_this()),
            last_message,
            counter));
        }
      }
    }
  }

  std::shared_ptr<const Node_Key> Node_Binary::get_key_left() const {
    return m_key_left;
  }

  std::shared_ptr<const Node_Key> Node_Binary::get_key_right() const {
    return m_key_right;
  }

  std::shared_ptr<const Node> Node_Binary::get_input_left() const {
    return m_input_left;
  }

  std::shared_ptr<Node> Node_Binary::get_input_left() {
    return m_input_left;
  }

  std::shared_ptr<const Node> Node_Binary::get_input_right() const {
    return m_input_right;
  }

  std::shared_ptr<Node> Node_Binary::get_input_right() {
    return m_input_right;
  }

  std::pair<Node::Node_Trie::Result, std::shared_ptr<Node>> Node_Binary::connect_new_or_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) {
    const auto sft = shared_from_this();

    assert(dynamic_cast<const Node_Key_Null *>(key.get()));

    const auto[result, snapshot, value] = m_join_layer_output_trie.insert_ip_xp<JOIN_LAYER_OUTPUTS_UNLINKED, JOIN_LAYER_OUTPUTS>(child);

    if (result == Node_Trie::Result::First_Insertion)
      job_queue->give_one(std::make_shared<Message_Connect_Join>(sft, network, std::move(snapshot), value));

    return std::make_pair(result, value);
  }

  Node::Node_Trie::Result Node_Binary::connect_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child, const bool unlinked) {
    const auto sft = shared_from_this();

    assert(dynamic_cast<const Node_Key_Null *>(key.get()));

    const auto[result, snapshot, value] = unlinked
      ? m_join_layer_output_trie.insert_ip_xp<JOIN_LAYER_OUTPUTS, JOIN_LAYER_OUTPUTS_UNLINKED>(child)
      : m_join_layer_output_trie.insert_ip_xp<JOIN_LAYER_OUTPUTS_UNLINKED, JOIN_LAYER_OUTPUTS>(child);

    assert(value == child);

    if (result == Node_Trie::Result::First_Insertion) {
      if (unlinked) {
        if (child->is_linked(sft, key))
          link_output(network, job_queue, key, child);
      }
      else
        job_queue->give_one(std::make_shared<Message_Connect_Join>(sft, network, std::move(snapshot), value));
    }

    return result;
  }

  std::shared_ptr<Node_Binary> Node_Binary::connect_created(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> input_left, const std::shared_ptr<Node> input_right, const std::shared_ptr<Node_Binary> created) {
    const auto multisym_left = std::dynamic_pointer_cast<const Node_Key_Multisym>(created->get_key_left());
    auto mt = multisym_left ? multisym_left->symbols.cbegin() : Node_Key_Multisym::Node_Key_Symbol_Trie::const_iterator();

    const auto[result, connected] = input_left->connect_new_or_existing_output(network, job_queue, multisym_left ? *mt++ : created->get_key_left(), created);
    if (result != Node_Trie::Result::First_Insertion)
      input_left->send_disconnect_from_parents(network, job_queue);

    const auto connected_join = std::static_pointer_cast<Node_Binary>(connected);
    assert(input_left == connected_join->get_input_left());
    assert(input_right == connected_join->get_input_right());

    if (multisym_left) {
      for (const auto mend = multisym_left->symbols.cend(); mt != mend; ++mt) {
        const auto result2 = input_left->connect_existing_output(network, job_queue, *mt, connected, false);
        if (result2 == Node_Trie::Result::First_Insertion)
          input_left->connect_to_parents_again(network, job_queue);
      }
    }

    if (connected_join->m_left_eq_right)
      input_right->send_disconnect_from_parents(network, job_queue);
    else {
      if (const auto multisym_right = std::dynamic_pointer_cast<const Node_Key_Multisym>(created->get_key_right())) {
        for (const auto sym : multisym_right->symbols) {
          const auto result2 = input_right->connect_existing_output(network, job_queue, sym, connected, true);
          if (result2 == Node_Trie::Result::First_Insertion)
            input_right->connect_to_parents_again(network, job_queue);
        }
        input_right->send_disconnect_from_parents(network, job_queue);
      }
      else {
        const auto result2 = input_right->connect_existing_output(network, job_queue, connected_join->get_key_right(), connected, true);
        if (result2 != Node_Trie::Result::First_Insertion)
          input_right->send_disconnect_from_parents(network, job_queue);
      }
    }

    return connected_join;
  }

  Node::Node_Trie::Result Node_Binary::link_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) {
    assert(std::dynamic_pointer_cast<const Node_Key_Null>(key));

    const auto[result, snapshot, value] = m_join_layer_output_trie.move<JOIN_LAYER_OUTPUTS_UNLINKED, JOIN_LAYER_OUTPUTS>(child);

    if (result != Node_Trie::Result::Successful_Move)
      return result;

    insert_tokens(network, job_queue, child, snapshot);

    return result;
  }

  Node::Node_Trie::Result Node_Binary::unlink_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) {
    assert(std::dynamic_pointer_cast<const Node_Key_Null>(key));

    const auto[result, snapshot, value] = m_join_layer_output_trie.move<JOIN_LAYER_OUTPUTS, JOIN_LAYER_OUTPUTS_UNLINKED>(child);

    if (result != Node_Trie::Result::Successful_Move)
      return result;

    remove_tokens(network, job_queue, child, snapshot);

    return result;
  }

  bool Node_Binary::has_tokens([[maybe_unused]] const std::shared_ptr<const Node_Key> key) const {
    assert(dynamic_cast<const Node_Key_Null *>(key.get()));

    const auto tokens = m_join_layer_output_trie.snapshot<JOIN_LAYER_OUTPUT_TOKENS>();
    return !tokens.size_zero();
  }

  void Node_Binary::receive(const Message_Connect_Join &message) {
    insert_tokens(message.network, message.get_Job_Queue(), message.child, message.snapshot);
  }

  void Node_Binary::receive(const Message_Disconnect_Output &message) {
    assert(std::dynamic_pointer_cast<const Node_Key_Null>(message.key));

    const auto[result, snapshot, value] = m_join_layer_output_trie.erase_ip_xp<JOIN_LAYER_OUTPUTS, JOIN_LAYER_OUTPUTS_UNLINKED>(message.child);

    assert(result != Node_Trie::Result::Failed_Removal);

    if (result != Node_Trie::Result::Last_Removal_IP && result != Node_Trie::Result::Last_Removal)
      return;

    send_disconnect_from_parents(message.network, message.get_Job_Queue());

    if (result != Node_Trie::Result::Last_Removal_IP)
      return;

    remove_tokens(message.network, message.get_Job_Queue(), message.child, snapshot);
  }

  void Node_Binary::insert_tokens(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> child, const Join_Layer_Snapshot snapshot) {
    const auto sft = shared_from_this();

    for (const auto &token : snapshot.snapshot<JOIN_LAYER_OUTPUT_TOKENS>())
      job_queue->give_one(std::make_shared<Message_Token_Insert>(child, network, sft, Node_Key_Null::Create(), token));
  }

  void Node_Binary::remove_tokens(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> child, const Join_Layer_Snapshot snapshot) {
    const auto sft = shared_from_this();

    for (const auto &token : snapshot.snapshot<JOIN_LAYER_OUTPUT_TOKENS>())
      job_queue->give_one(std::make_shared<Message_Token_Remove>(child, network, sft, Node_Key_Null::Create(), token));
  }

  bool Node_Binary::is_linked(const std::shared_ptr<Node> input, const std::shared_ptr<const Node_Key> key) {
    assert(!m_left_eq_right);

    const Concurrency::Intrusive_Shared_Ptr<Link_Data>::Lock link_data = m_link_data.load(std::memory_order_acquire);
    assert(link_data);

    if (link_data->link_status == Link_Status::BOTH_LINKED)
      return true;
    else if (link_data->link_status == Link_Status::LEFT_UNLINKED) {
      if (input != m_input_left || *key != *m_key_left)
        return true;
      else {
        const auto multisym_left = std::dynamic_pointer_cast<const Node_Key_Multisym>(m_key_left);

        if (multisym_left) {
          for (const auto sym_left : multisym_left->symbols) {
            if (m_input_left->has_tokens(sym_left))
              return true;
          }
          return false;
        }
        else
          return m_input_left->has_tokens(m_key_left);
      }
    }
    else {
      if (input != m_input_right || *key != *m_key_right)
        return true;
      else {
        const auto multisym_right = std::dynamic_pointer_cast<const Node_Key_Multisym>(m_key_right);

        if (multisym_right) {
          for (const auto sym_right : multisym_right->symbols) {
            if (m_input_right->has_tokens(sym_right))
              return true;
          }
          return false;
        }
        else
          return m_input_right->has_tokens(m_key_right);
      }
    }
  }

  std::shared_ptr<Symbols> Node_Binary::bind_variables_left(const std::shared_ptr<const Token> token_left) const {
    auto symbols = std::make_shared<Symbols>();
    symbols->reserve(m_variable_bindings.size());
    for (auto variable_binding : m_variable_bindings)
      symbols->emplace_back((*token_left)[variable_binding.first]);
    return symbols;
  }

  std::shared_ptr<Symbols> Node_Binary::bind_variables_right(const std::shared_ptr<const Token> token_right) const {
    auto symbols = std::make_shared<Symbols>();
    symbols->reserve(m_variable_bindings.size());
    for (auto variable_binding : m_variable_bindings)
      symbols->emplace_back((*token_right)[variable_binding.second]);
    return symbols;
  }

}
