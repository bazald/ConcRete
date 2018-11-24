#include "Zeni/Rete/Node_Filter_2.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Internal/Message_Connect_Filter_2.hpp"
#include "Zeni/Rete/Internal/Message_Disconnect_Output.hpp"
#include "Zeni/Rete/Internal/Message_Token_Insert.hpp"
#include "Zeni/Rete/Internal/Message_Token_Remove.hpp"
#include "Zeni/Rete/Internal/Token_Alpha.hpp"
#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Node_Key.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Filter_2::Node_Filter_2(const std::shared_ptr<Network> network, const std::shared_ptr<const Node_Key> node_key, const std::shared_ptr<Node> input)
    : Node_Unary(1, 1, 1, hash_combine(std::hash<int>()(2), node_key->hash()), node_key, input)
  {
    assert(dynamic_cast<const Node_Key_Symbol *>(node_key.get())
      || dynamic_cast<const Node_Key_01 *>(node_key.get())
      || dynamic_cast<const Node_Key_Multisym *>(node_key.get()));
  }

  std::shared_ptr<Node_Filter_2> Node_Filter_2::Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> node_key, const std::shared_ptr<Node> input) {
    class Friendly_Node_Filter_2 : public Node_Filter_2 {
    public:
      Friendly_Node_Filter_2(const std::shared_ptr<Network> &network, const std::shared_ptr<const Node_Key> node_key, const std::shared_ptr<Node> input) : Node_Filter_2(network, node_key, input) {}
    };

    const auto created = std::shared_ptr<Friendly_Node_Filter_2>(new Friendly_Node_Filter_2(network, node_key, input));

    const auto multisym = std::dynamic_pointer_cast<const Node_Key_Multisym>(node_key);
    auto mt = multisym ? multisym->symbols.cbegin() : Node_Key_Multisym::Node_Key_Symbol_Trie::const_iterator();

    const auto [result, connected] = input->connect_new_or_existing_output(network, job_queue, multisym ? *mt++ : node_key, created);
    if (result != Node_Trie::Result::First_Insertion)
      input->send_disconnect_from_parents(network, job_queue);

    if (multisym) {
      for (const auto mend = multisym->symbols.cend(); mt != mend; ++mt) {
        const auto result2 = input->connect_existing_output(network, job_queue, *mt, connected, false);
        if (result2 == Node_Trie::Result::First_Insertion)
          input->connect_to_parents_again(network, job_queue);
      }
    }

    return std::static_pointer_cast<Node_Filter_2>(connected);
  }

  std::pair<Node::Node_Trie::Result, std::shared_ptr<Node>> Node_Filter_2::connect_new_or_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) {
    const auto sft = shared_from_this();

    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(key);
    assert(key_symbol || dynamic_cast<const Node_Key_02 *>(key.get()) || dynamic_cast<const Node_Key_12 *>(key.get()));

    const auto[result, snapshot, value] = key_symbol
      ? m_filter_layer_2_trie.insert_2_ip_xp<FILTER_LAYER_2_SYMBOL, FILTER_LAYER_2_SYMBOL_OUTPUTS_UNLINKED, FILTER_LAYER_2_SYMBOL_OUTPUTS>(key_symbol->symbol, child)
      : dynamic_cast<const Node_Key_02 *>(key.get())
      ? m_filter_layer_2_trie.insert_ip_xp<FILTER_LAYER_2_VARIABLE_OUTPUTS_02_UNLINKED, FILTER_LAYER_2_VARIABLE_OUTPUTS_02>(child)
      : m_filter_layer_2_trie.insert_ip_xp<FILTER_LAYER_2_VARIABLE_OUTPUTS_12_UNLINKED, FILTER_LAYER_2_VARIABLE_OUTPUTS_12>(child);

    if (result == Node_Trie::Result::First_Insertion)
      job_queue->give_one(std::make_shared<Message_Connect_Filter_2>(sft, network, std::move(snapshot), key, value));

    return std::make_pair(result, value);
  }

  Node::Node_Trie::Result Node_Filter_2::connect_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child, const bool unlinked) {
    const auto sft = shared_from_this();

    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(key);
    assert(key_symbol || dynamic_cast<const Node_Key_02 *>(key.get()) || dynamic_cast<const Node_Key_12 *>(key.get()));

    const auto[result, snapshot, value] = unlinked
      ? (key_symbol
        ? m_filter_layer_2_trie.insert_2_ip_xp<FILTER_LAYER_2_SYMBOL, FILTER_LAYER_2_SYMBOL_OUTPUTS, FILTER_LAYER_2_SYMBOL_OUTPUTS_UNLINKED>(key_symbol->symbol, child)
        : dynamic_cast<const Node_Key_02 *>(key.get())
        ? m_filter_layer_2_trie.insert_ip_xp<FILTER_LAYER_2_VARIABLE_OUTPUTS_02, FILTER_LAYER_2_VARIABLE_OUTPUTS_02_UNLINKED>(child)
        : m_filter_layer_2_trie.insert_ip_xp<FILTER_LAYER_2_VARIABLE_OUTPUTS_12, FILTER_LAYER_2_VARIABLE_OUTPUTS_12_UNLINKED>(child))
      : (key_symbol
        ? m_filter_layer_2_trie.insert_2_ip_xp<FILTER_LAYER_2_SYMBOL, FILTER_LAYER_2_SYMBOL_OUTPUTS_UNLINKED, FILTER_LAYER_2_SYMBOL_OUTPUTS>(key_symbol->symbol, child)
        : dynamic_cast<const Node_Key_02 *>(key.get())
        ? m_filter_layer_2_trie.insert_ip_xp<FILTER_LAYER_2_VARIABLE_OUTPUTS_02_UNLINKED, FILTER_LAYER_2_VARIABLE_OUTPUTS_02>(child)
        : m_filter_layer_2_trie.insert_ip_xp<FILTER_LAYER_2_VARIABLE_OUTPUTS_12_UNLINKED, FILTER_LAYER_2_VARIABLE_OUTPUTS_12>(child));

    assert(value == child);

    if (!unlinked && result == Node_Trie::Result::First_Insertion)
      job_queue->give_one(std::make_shared<Message_Connect_Filter_2>(sft, network, std::move(snapshot), key, value));

    return result;
  }

  Node::Node_Trie::Result Node_Filter_2::link_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) {
    const auto sft = shared_from_this();

    const auto key_node = key;
    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(key_node);
    assert(key_symbol || std::dynamic_pointer_cast<const Node_Key_02>(key_node) || std::dynamic_pointer_cast<const Node_Key_12>(key_node));

    const auto[result, snapshot, value] = key_symbol
      ? m_filter_layer_2_trie.move_2<FILTER_LAYER_2_SYMBOL, FILTER_LAYER_2_SYMBOL_OUTPUTS_UNLINKED, FILTER_LAYER_2_SYMBOL_OUTPUTS>(key_symbol->symbol, child)
      : dynamic_cast<const Node_Key_02 *>(key.get())
      ? m_filter_layer_2_trie.move<FILTER_LAYER_2_VARIABLE_OUTPUTS_02_UNLINKED, FILTER_LAYER_2_VARIABLE_OUTPUTS_02>(child)
      : m_filter_layer_2_trie.move<FILTER_LAYER_2_VARIABLE_OUTPUTS_12_UNLINKED, FILTER_LAYER_2_VARIABLE_OUTPUTS_12>(child);

    if (result != Node_Trie::Result::Successful_Move)
      return result;

    insert_tokens(network, job_queue, key, child, snapshot);

    return result;
  }

  Node::Node_Trie::Result Node_Filter_2::unlink_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) {
    const auto key_node = key;
    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(key_node);
    assert(key_symbol || std::dynamic_pointer_cast<const Node_Key_02>(key_node) || std::dynamic_pointer_cast<const Node_Key_12>(key_node));

    const auto[result, snapshot, value] = key_symbol
      ? m_filter_layer_2_trie.move_2<FILTER_LAYER_2_SYMBOL, FILTER_LAYER_2_SYMBOL_OUTPUTS, FILTER_LAYER_2_SYMBOL_OUTPUTS_UNLINKED>(key_symbol->symbol, child)
      : dynamic_cast<const Node_Key_02 *>(key.get())
      ? m_filter_layer_2_trie.move<FILTER_LAYER_2_VARIABLE_OUTPUTS_02, FILTER_LAYER_2_VARIABLE_OUTPUTS_02_UNLINKED>(child)
      : m_filter_layer_2_trie.move<FILTER_LAYER_2_VARIABLE_OUTPUTS_12, FILTER_LAYER_2_VARIABLE_OUTPUTS_12_UNLINKED>(child);

    if (result != Node_Trie::Result::Successful_Move)
      return result;

    remove_tokens(network, job_queue, key, child, snapshot);

    return result;
  }

  bool Node_Filter_2::has_tokens(const std::shared_ptr<const Node_Key> key) const {
    if (const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(key)) {
      const auto tokens = m_filter_layer_2_trie.lookup_snapshot<FILTER_LAYER_2_SYMBOL, FILTER_LAYER_2_SYMBOL_TOKENS>(key_symbol->symbol);
      return !tokens.size_zero();
    }
    else {
      assert(dynamic_cast<const Node_Key_02 *>(key.get()) || dynamic_cast<const Node_Key_12 *>(key.get()));

      const auto snapshot = m_filter_layer_2_trie.snapshot();
      for (const auto tokens : snapshot.snapshot<FILTER_LAYER_2_SYMBOL>()) {
        if (!tokens.second.size_zero<FILTER_LAYER_2_SYMBOL_TOKENS>())
          return true;
      }
      return false;
    }
  }

  void Node_Filter_2::receive(const Message_Token_Insert &message) {
    const auto &wme = *std::dynamic_pointer_cast<const Token_Alpha>(message.token)->get_wme();
    const auto &symbols = wme.get_symbols();
    const auto &symbol = std::get<2>(symbols);

    const auto[result, snapshot, value] = m_filter_layer_2_trie.insert_2<FILTER_LAYER_2_SYMBOL, FILTER_LAYER_2_SYMBOL_TOKENS>(symbol, message.token);
    if (result != Token_Trie::Result::First_Insertion)
      return;

    const auto sft = shared_from_this();
    const auto &job_queue = message.get_Job_Queue();

    for (auto &output : snapshot.lookup_snapshot<FILTER_LAYER_2_SYMBOL, FILTER_LAYER_2_SYMBOL_OUTPUTS>(symbol))
      job_queue->give_one(std::make_shared<Message_Token_Insert>(output, message.network, sft, Node_Key_Symbol::Create(symbol), message.token));
    if (*std::get<0>(symbols) == *std::get<2>(symbols)) {
      for (auto &output : snapshot.snapshot<FILTER_LAYER_2_VARIABLE_OUTPUTS_02>())
        job_queue->give_one(std::make_shared<Message_Token_Insert>(output, message.network, sft, Node_Key_02::Create(), message.token));
    }
    if (*std::get<1>(symbols) == *std::get<2>(symbols)) {
      for (auto &output : snapshot.snapshot<FILTER_LAYER_2_VARIABLE_OUTPUTS_12>())
        job_queue->give_one(std::make_shared<Message_Token_Insert>(output, message.network, sft, Node_Key_12::Create(), message.token));
    }
  }

  void Node_Filter_2::receive(const Message_Token_Remove &message) {
    const auto &wme = *std::dynamic_pointer_cast<const Token_Alpha>(message.token)->get_wme();
    const auto &symbols = wme.get_symbols();
    const auto &symbol = std::get<2>(wme.get_symbols());

    const auto[result, snapshot, value] = m_filter_layer_2_trie.erase_2<FILTER_LAYER_2_SYMBOL, FILTER_LAYER_2_SYMBOL_TOKENS>(symbol, message.token);
    if (result != Token_Trie::Result::Last_Removal)
      return;

    const auto sft = shared_from_this();
    const auto &job_queue = message.get_Job_Queue();

    for (auto &output : snapshot.lookup_snapshot<FILTER_LAYER_2_SYMBOL, FILTER_LAYER_2_SYMBOL_OUTPUTS>(symbol))
      job_queue->give_one(std::make_shared<Message_Token_Remove>(output, message.network, sft, Node_Key_Symbol::Create(symbol), message.token));
    if (*std::get<0>(symbols) == *std::get<2>(symbols)) {
      for (auto &output : snapshot.snapshot<FILTER_LAYER_2_VARIABLE_OUTPUTS_02>())
        job_queue->give_one(std::make_shared<Message_Token_Remove>(output, message.network, sft, Node_Key_02::Create(), message.token));
    }
    if (*std::get<1>(symbols) == *std::get<2>(symbols)) {
      for (auto &output : snapshot.snapshot<FILTER_LAYER_2_VARIABLE_OUTPUTS_12>())
        job_queue->give_one(std::make_shared<Message_Token_Remove>(output, message.network, sft, Node_Key_12::Create(), message.token));
    }
  }

  void Node_Filter_2::receive(const Message_Connect_Filter_2 &message) {
    insert_tokens(message.network, message.get_Job_Queue(), message.key, message.child, message.snapshot);
  }

  void Node_Filter_2::receive(const Message_Disconnect_Output &message) {
    const auto key_node = message.key;
    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(key_node);
    assert(key_symbol || std::dynamic_pointer_cast<const Node_Key_02>(key_node) || std::dynamic_pointer_cast<const Node_Key_12>(key_node));

    const auto[result, snapshot, value] = key_symbol
      ? m_filter_layer_2_trie.erase_2_ip_xp<FILTER_LAYER_2_SYMBOL, FILTER_LAYER_2_SYMBOL_OUTPUTS, FILTER_LAYER_2_SYMBOL_OUTPUTS_UNLINKED>(key_symbol->symbol, message.child)
      : dynamic_cast<const Node_Key_02 *>(key_node.get())
      ? m_filter_layer_2_trie.erase_ip_xp<FILTER_LAYER_2_VARIABLE_OUTPUTS_02, FILTER_LAYER_2_VARIABLE_OUTPUTS_02_UNLINKED>(message.child)
      : m_filter_layer_2_trie.erase_ip_xp<FILTER_LAYER_2_VARIABLE_OUTPUTS_12, FILTER_LAYER_2_VARIABLE_OUTPUTS_12_UNLINKED>(message.child);

    assert(result != Node_Trie::Result::Failed_Removal);

    if (result != Node_Trie::Result::Last_Removal_IP && result != Node_Trie::Result::Last_Removal)
      return;

    send_disconnect_from_parents(message.network, message.get_Job_Queue());

    if (result != Node_Trie::Result::Last_Removal_IP)
      return;

    remove_tokens(message.network, message.get_Job_Queue(), message.key, message.child, snapshot);
  }

  void Node_Filter_2::insert_tokens(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child, const Filter_Layer_2_Snapshot snapshot) {
    const auto sft = shared_from_this();

    const auto key_node = key;
    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(key_node);
    assert(key_symbol || std::dynamic_pointer_cast<const Node_Key_02>(key_node) || std::dynamic_pointer_cast<const Node_Key_12>(key_node));

    if (key_symbol) {
      for (const auto &token : snapshot.lookup_snapshot<FILTER_LAYER_2_SYMBOL, FILTER_LAYER_2_SYMBOL_TOKENS>(key_symbol->symbol))
        job_queue->give_one(std::make_shared<Message_Token_Insert>(child, network, sft, Node_Key_Symbol::Create(key_symbol->symbol), token));
    }
    else if (dynamic_cast<const Node_Key_02 *>(key_node.get())) {
      for (const auto tokens : snapshot.snapshot<FILTER_LAYER_2_SYMBOL>()) {
        for (const auto &token : tokens.second.snapshot<FILTER_LAYER_2_SYMBOL_TOKENS>()) {
          const auto token_alpha = std::dynamic_pointer_cast<const Token_Alpha>(token);
          const auto &symbols = token_alpha->get_wme()->get_symbols();
          if (*std::get<0>(symbols) == *std::get<2>(symbols))
            job_queue->give_one(std::make_shared<Message_Token_Insert>(child, network, sft, Node_Key_02::Create(), token));
        }
      }
    }
    else {
      for (const auto tokens : snapshot.snapshot<FILTER_LAYER_2_SYMBOL>()) {
        for (const auto &token : tokens.second.snapshot<FILTER_LAYER_2_SYMBOL_TOKENS>()) {
          const auto token_alpha = std::dynamic_pointer_cast<const Token_Alpha>(token);
          const auto &symbols = token_alpha->get_wme()->get_symbols();
          if (*std::get<1>(symbols) == *std::get<2>(symbols))
            job_queue->give_one(std::make_shared<Message_Token_Insert>(child, network, sft, Node_Key_12::Create(), token));
        }
      }
    }
  }

  void Node_Filter_2::remove_tokens(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child, const Filter_Layer_2_Snapshot snapshot) {
    const auto sft = shared_from_this();

    const auto key_node = key;
    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(key_node);
    assert(key_symbol || std::dynamic_pointer_cast<const Node_Key_02>(key_node) || std::dynamic_pointer_cast<const Node_Key_12>(key_node));

    if (key_symbol) {
      for (const auto &token : snapshot.lookup_snapshot<FILTER_LAYER_2_SYMBOL, FILTER_LAYER_2_SYMBOL_TOKENS>(key_symbol->symbol))
        job_queue->give_one(std::make_shared<Message_Token_Remove>(child, network, sft, Node_Key_Symbol::Create(key_symbol->symbol), token));
    }
    else if (dynamic_cast<const Node_Key_02 *>(key_node.get())) {
      for (const auto tokens : snapshot.snapshot<FILTER_LAYER_2_SYMBOL>()) {
        for (const auto &token : tokens.second.snapshot<FILTER_LAYER_2_SYMBOL_TOKENS>()) {
          const auto token_alpha = std::dynamic_pointer_cast<const Token_Alpha>(token);
          const auto &symbols = token_alpha->get_wme()->get_symbols();
          if (*std::get<0>(symbols) == *std::get<2>(symbols))
            job_queue->give_one(std::make_shared<Message_Token_Remove>(child, network, sft, Node_Key_02::Create(), token));
        }
      }
    }
    else {
      for (const auto tokens : snapshot.snapshot<FILTER_LAYER_2_SYMBOL>()) {
        for (const auto &token : tokens.second.snapshot<FILTER_LAYER_2_SYMBOL_TOKENS>()) {
          const auto token_alpha = std::dynamic_pointer_cast<const Token_Alpha>(token);
          const auto &symbols = token_alpha->get_wme()->get_symbols();
          if (*std::get<1>(symbols) == *std::get<2>(symbols))
            job_queue->give_one(std::make_shared<Message_Token_Remove>(child, network, sft, Node_Key_12::Create(), token));
        }
      }
    }
  }

  bool Node_Filter_2::operator==(const Node &rhs) const {
    //return this == &rhs;

    if (auto filter_2 = dynamic_cast<const Node_Filter_2 *>(&rhs)) {
      return *get_key() == *filter_2->get_key() && get_input() == filter_2->get_input();
    }

    return false;
  }

}
