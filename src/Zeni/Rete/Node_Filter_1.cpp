#include "Zeni/Rete/Node_Filter_1.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Internal/Message_Connect_Filter_1.hpp"
#include "Zeni/Rete/Internal/Message_Disconnect_Output.hpp"
#include "Zeni/Rete/Internal/Message_Token_Insert.hpp"
#include "Zeni/Rete/Internal/Message_Token_Remove.hpp"
#include "Zeni/Rete/Internal/Token_Alpha.hpp"
#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Node_Key.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Filter_1::Node_Filter_1(const std::shared_ptr<Network> network, const std::shared_ptr<const Node_Key> node_key)
    : Node_Unary(1, 1, 1, hash_combine(std::hash<int>()(1), node_key->hash()), node_key, network)
  {
    assert(dynamic_cast<const Node_Key_Symbol *>(node_key.get())
      || dynamic_cast<const Node_Key_Null *>(node_key.get())
      || dynamic_cast<const Node_Key_Multisym *>(node_key.get()));
  }

  std::shared_ptr<Node_Filter_1> Node_Filter_1::Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> node_key) {
    class Friendly_Node_Filter_1 : public Node_Filter_1 {
    public:
      Friendly_Node_Filter_1(const std::shared_ptr<Network> &network, const std::shared_ptr<const Node_Key> node_key) : Node_Filter_1(network, node_key) {}
    };

    const auto created = std::shared_ptr<Friendly_Node_Filter_1>(new Friendly_Node_Filter_1(network, node_key));

    const auto multisym = std::dynamic_pointer_cast<const Node_Key_Multisym>(node_key);
    auto mt = multisym ? multisym->symbols.cbegin() : Node_Key_Multisym::Node_Key_Symbol_Trie::const_iterator();

    const auto [result, connected] = network->connect_new_or_existing_output(network, job_queue, multisym ? *mt++ : node_key, created);

    if (multisym) {
      for (const auto mend = multisym->symbols.cend(); mt != mend; ++mt)
        [[maybe_unused]] const auto result2 = network->connect_existing_output(network, job_queue, *mt, connected, false);
    }

    return std::static_pointer_cast<Node_Filter_1>(connected);
  }

  std::pair<Node::Node_Trie::Result, std::shared_ptr<Node>> Node_Filter_1::connect_new_or_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) {
    const auto sft = shared_from_this();

    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(key);
    assert(key_symbol || dynamic_cast<const Node_Key_01 *>(key.get()));

    const auto[result, snapshot, value] = key_symbol
      ? m_filter_layer_1_trie.insert_2_ip_xp<FILTER_LAYER_1_SYMBOL, FILTER_LAYER_1_SYMBOL_OUTPUTS_UNLINKED, FILTER_LAYER_1_SYMBOL_OUTPUTS>(key_symbol->symbol, child)
      : m_filter_layer_1_trie.insert_ip_xp<FILTER_LAYER_1_VARIABLE_OUTPUTS_01_UNLINKED, FILTER_LAYER_1_VARIABLE_OUTPUTS_01>(child);

    if (result == Node_Trie::Result::First_Insertion)
      job_queue->give_one(std::make_shared<Message_Connect_Filter_1>(sft, network, std::move(snapshot), key, value));

    return std::make_pair(result, value);
  }

  Node::Node_Trie::Result Node_Filter_1::connect_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child, const bool unlinked) {
    const auto sft = shared_from_this();

    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(key);
    assert(key_symbol || dynamic_cast<const Node_Key_01 *>(key.get()));

    const auto[result, snapshot, value] =
      unlinked
      ? (key_symbol
        ? m_filter_layer_1_trie.insert_2_ip_xp<FILTER_LAYER_1_SYMBOL, FILTER_LAYER_1_SYMBOL_OUTPUTS, FILTER_LAYER_1_SYMBOL_OUTPUTS_UNLINKED>(key_symbol->symbol, child)
        : m_filter_layer_1_trie.insert_ip_xp<FILTER_LAYER_1_VARIABLE_OUTPUTS_01, FILTER_LAYER_1_VARIABLE_OUTPUTS_01_UNLINKED>(child))
      : (key_symbol
        ? m_filter_layer_1_trie.insert_2_ip_xp<FILTER_LAYER_1_SYMBOL, FILTER_LAYER_1_SYMBOL_OUTPUTS_UNLINKED, FILTER_LAYER_1_SYMBOL_OUTPUTS>(key_symbol->symbol, child)
        : m_filter_layer_1_trie.insert_ip_xp<FILTER_LAYER_1_VARIABLE_OUTPUTS_01_UNLINKED, FILTER_LAYER_1_VARIABLE_OUTPUTS_01>(child));

    assert(value == child);

    if (!unlinked && result == Node_Trie::Result::First_Insertion)
      job_queue->give_one(std::make_shared<Message_Connect_Filter_1>(sft, network, std::move(snapshot), key, value));

    return result;
  }

  Node::Node_Trie::Result Node_Filter_1::link_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) {
    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(key);

    const auto[result, snapshot, value] = key_symbol
      ? m_filter_layer_1_trie.move_2<FILTER_LAYER_1_SYMBOL, FILTER_LAYER_1_SYMBOL_OUTPUTS_UNLINKED, FILTER_LAYER_1_SYMBOL_OUTPUTS>(key_symbol->symbol, child)
      : m_filter_layer_1_trie.move<FILTER_LAYER_1_VARIABLE_OUTPUTS_01_UNLINKED, FILTER_LAYER_1_VARIABLE_OUTPUTS_01>(child);

    if (result != Node_Trie::Result::Successful_Move)
      return result;

    insert_tokens(network, job_queue, key, child, snapshot);

    return result;
  }

  Node::Node_Trie::Result Node_Filter_1::unlink_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) {
    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(key);

    const auto[result, snapshot, value] = key_symbol
      ? m_filter_layer_1_trie.move_2<FILTER_LAYER_1_SYMBOL, FILTER_LAYER_1_SYMBOL_OUTPUTS, FILTER_LAYER_1_SYMBOL_OUTPUTS_UNLINKED>(key_symbol->symbol, child)
      : m_filter_layer_1_trie.move<FILTER_LAYER_1_VARIABLE_OUTPUTS_01, FILTER_LAYER_1_VARIABLE_OUTPUTS_01_UNLINKED>(child);

    if (result != Node_Trie::Result::Successful_Move)
      return result;

    remove_tokens(network, job_queue, key, child, snapshot);

    return result;
  }

  bool Node_Filter_1::has_tokens(const std::shared_ptr<const Node_Key> key) const {
    if (const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(key)) {
      const auto tokens = m_filter_layer_1_trie.lookup_snapshot<FILTER_LAYER_1_SYMBOL, FILTER_LAYER_1_SYMBOL_TOKENS>(key_symbol->symbol);
      return !tokens.size_zero();
    }
    else {
      assert(dynamic_cast<const Node_Key_01 *>(key.get()));

      const auto snapshot = m_filter_layer_1_trie.snapshot();
      for (const auto tokens : snapshot.snapshot<FILTER_LAYER_1_SYMBOL>()) {
        if (!tokens.second.size_zero<FILTER_LAYER_1_SYMBOL_TOKENS>())
          return true;
      }
      return false;
    }
  }

  void Node_Filter_1::receive(const Message_Token_Insert &message) {
    const auto &wme = *std::dynamic_pointer_cast<const Token_Alpha>(message.token)->get_wme();
    const auto &symbols = wme.get_symbols();
    const auto &symbol = std::get<1>(symbols);

    const auto[result, snapshot, value] = m_filter_layer_1_trie.insert_2<FILTER_LAYER_1_SYMBOL, FILTER_LAYER_1_SYMBOL_TOKENS>(symbol, message.token);
    if (result != Token_Trie::Result::First_Insertion)
      return;

    const auto sft = shared_from_this();
    const auto &job_queue = message.get_Job_Queue();

    for (auto &output : snapshot.lookup_snapshot<FILTER_LAYER_1_SYMBOL, FILTER_LAYER_1_SYMBOL_OUTPUTS>(symbol))
      job_queue->give_one(std::make_shared<Message_Token_Insert>(output, message.network, sft, Node_Key_Symbol::Create(symbol), message.token));
    if (*std::get<0>(symbols) == *std::get<1>(symbols)) {
      for (auto &output : snapshot.snapshot<FILTER_LAYER_1_VARIABLE_OUTPUTS_01>())
        job_queue->give_one(std::make_shared<Message_Token_Insert>(output, message.network, sft, Node_Key_01::Create(), message.token));
    }
  }

  void Node_Filter_1::receive(const Message_Token_Remove &message) {
    const auto &wme = *std::dynamic_pointer_cast<const Token_Alpha>(message.token)->get_wme();
    const auto &symbols = wme.get_symbols();
    const auto &symbol = std::get<1>(wme.get_symbols());

    const auto[result, snapshot, value] = m_filter_layer_1_trie.erase_2<FILTER_LAYER_1_SYMBOL, FILTER_LAYER_1_SYMBOL_TOKENS>(symbol, message.token);
    if (result != Token_Trie::Result::Last_Removal)
      return;

    const auto sft = shared_from_this();
    const auto &job_queue = message.get_Job_Queue();

    for (auto &output : snapshot.lookup_snapshot<FILTER_LAYER_1_SYMBOL, FILTER_LAYER_1_SYMBOL_OUTPUTS>(symbol))
      job_queue->give_one(std::make_shared<Message_Token_Remove>(output, message.network, sft, Node_Key_Symbol::Create(symbol), message.token));
    if (*std::get<0>(symbols) == *std::get<1>(symbols)) {
      for (auto &output : snapshot.snapshot<FILTER_LAYER_1_VARIABLE_OUTPUTS_01>())
        job_queue->give_one(std::make_shared<Message_Token_Remove>(output, message.network, sft, Node_Key_01::Create(), message.token));
    }
  }

  void Node_Filter_1::receive(const Message_Connect_Filter_1 &message) {
    insert_tokens(message.network, message.get_Job_Queue(), message.key, message.child, message.snapshot);
  }

  void Node_Filter_1::receive(const Message_Disconnect_Output &message) {
    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(message.key);

    const auto[result, snapshot, value] = key_symbol
      ? m_filter_layer_1_trie.erase_2_ip_xp<FILTER_LAYER_1_SYMBOL, FILTER_LAYER_1_SYMBOL_OUTPUTS, FILTER_LAYER_1_SYMBOL_OUTPUTS_UNLINKED>(key_symbol->symbol, message.child)
      : m_filter_layer_1_trie.erase_ip_xp<FILTER_LAYER_1_VARIABLE_OUTPUTS_01, FILTER_LAYER_1_VARIABLE_OUTPUTS_01_UNLINKED>(message.child);

    assert(result != Node_Trie::Result::Failed_Removal);

    if (result != Node_Trie::Result::Last_Removal_IP && result != Node_Trie::Result::Last_Removal)
      return;

    send_disconnect_from_parents(message.network, message.get_Job_Queue());

    if (result != Node_Trie::Result::Last_Removal_IP)
      return;

    remove_tokens(message.network, message.get_Job_Queue(), message.key, message.child, snapshot);
  }

  void Node_Filter_1::insert_tokens(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child, const Filter_Layer_1_Snapshot snapshot) {
    const auto sft = shared_from_this();

    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(key);

    if (key_symbol) {
      for (const auto &token : snapshot.lookup_snapshot<FILTER_LAYER_1_SYMBOL, FILTER_LAYER_1_SYMBOL_TOKENS>(key_symbol->symbol))
        job_queue->give_one(std::make_shared<Message_Token_Insert>(child, network, sft, Node_Key_Symbol::Create(key_symbol->symbol), token));
    }
    else {
      for (const auto tokens : snapshot.snapshot<FILTER_LAYER_1_SYMBOL>()) {
        for (const auto &token : tokens.second.snapshot<FILTER_LAYER_1_SYMBOL_TOKENS>()) {
          const auto token_alpha = std::dynamic_pointer_cast<const Token_Alpha>(token);
          const auto &symbols = token_alpha->get_wme()->get_symbols();
          if (*std::get<0>(symbols) == *std::get<1>(symbols))
            job_queue->give_one(std::make_shared<Message_Token_Insert>(child, network, sft, Node_Key_01::Create(), token));
        }
      }
    }
  }

  void Node_Filter_1::remove_tokens(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child, const Filter_Layer_1_Snapshot snapshot) {
    const auto sft = shared_from_this();

    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(key);

    if (key_symbol) {
      for (const auto &token : snapshot.lookup_snapshot<FILTER_LAYER_1_SYMBOL, FILTER_LAYER_1_SYMBOL_TOKENS>(key_symbol->symbol))
        job_queue->give_one(std::make_shared<Message_Token_Remove>(child, network, sft, Node_Key_Symbol::Create(key_symbol->symbol), token));
    }
    else {
      for (const auto tokens : snapshot.snapshot<FILTER_LAYER_1_SYMBOL>()) {
        for (const auto &token : tokens.second.snapshot<FILTER_LAYER_1_SYMBOL_TOKENS>()) {
          const auto token_alpha = std::dynamic_pointer_cast<const Token_Alpha>(token);
          const auto &symbols = token_alpha->get_wme()->get_symbols();
          if (*std::get<0>(symbols) == *std::get<1>(symbols))
            job_queue->give_one(std::make_shared<Message_Token_Remove>(child, network, sft, Node_Key_01::Create(), token));
        }
      }
    }
  }

  bool Node_Filter_1::operator==(const Node &rhs) const {
    //return this == &rhs;

    if (auto filter_1 = dynamic_cast<const Node_Filter_1 *>(&rhs)) {
      return *get_key() == *filter_1->get_key();
    }

    return false;
  }

}
