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
    assert(dynamic_cast<const Node_Key_Symbol_0 *>(node_key.get())
      || dynamic_cast<const Node_Key_Symbol_1 *>(node_key.get())
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

    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol_2>(key);
    assert(key_symbol);

    const auto[result, snapshot, value] = m_filter_layer_trie.insert_ip_xp<FILTER_LAYER_OUTPUTS_UNLINKED, FILTER_LAYER_OUTPUTS>(key_symbol->symbol, child);

    if (result == Node_Trie::Result::First_Insertion)
      job_queue->give_one(std::make_shared<Message_Connect_Filter_2>(sft, network, std::move(snapshot), key, value));

    return std::make_pair(result, value);
  }

  Node::Node_Trie::Result Node_Filter_2::connect_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child, const bool unlinked) {
    const auto sft = shared_from_this();

    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol_2>(key);
    assert(key_symbol);

    const auto[result, snapshot, value] = unlinked
      ? m_filter_layer_trie.insert_ip_xp<FILTER_LAYER_OUTPUTS, FILTER_LAYER_OUTPUTS_UNLINKED>(key_symbol->symbol, child)
      : m_filter_layer_trie.insert_ip_xp<FILTER_LAYER_OUTPUTS_UNLINKED, FILTER_LAYER_OUTPUTS>(key_symbol->symbol, child);

    assert(value == child);

    if (result == Node_Trie::Result::First_Insertion) {
      if (unlinked) {
        if (child->is_linked(sft, key))
          link_output(network, job_queue, key, child);
      }
      else
        job_queue->give_one(std::make_shared<Message_Connect_Filter_2>(sft, network, std::move(snapshot), key, value));
    }

    return result;
  }

  Node::Node_Trie::Result Node_Filter_2::link_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) {
    const auto sft = shared_from_this();

    const auto key_node = key;
    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol_2>(key_node);
    assert(key_symbol);

    const auto[result, snapshot, value] = m_filter_layer_trie.move<FILTER_LAYER_OUTPUTS_UNLINKED, FILTER_LAYER_OUTPUTS>(key_symbol->symbol, child);

    if (result != Node_Trie::Result::Successful_Move)
      return result;

    insert_tokens(network, job_queue, key, child, snapshot);

    return result;
  }

  Node::Node_Trie::Result Node_Filter_2::unlink_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) {
    const auto key_node = key;
    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol_2>(key_node);
    assert(key_symbol);

    const auto[result, snapshot, value] = m_filter_layer_trie.move<FILTER_LAYER_OUTPUTS, FILTER_LAYER_OUTPUTS_UNLINKED>(key_symbol->symbol, child);

    if (result != Node_Trie::Result::Successful_Move)
      return result;

    remove_tokens(network, job_queue, key, child, snapshot);

    return result;
  }

  bool Node_Filter_2::has_tokens(const std::shared_ptr<const Node_Key> key) const {
    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol_2>(key);
    assert(key_symbol);

    const auto [result, tokens] = m_filter_layer_trie.snapshot<FILTER_LAYER_TOKENS>(key_symbol->symbol);
    return result == decltype(result)::Found && !tokens.size_zero();
  }

  void Node_Filter_2::receive(const Message_Token_Insert &message) {
    const auto &wme = *std::dynamic_pointer_cast<const Token_Alpha>(message.token)->get_wme();
    const auto &symbols = wme.get_symbols();
    const auto &symbol = std::get<2>(symbols);

    const auto[result, snapshot, value] = m_filter_layer_trie.insert<FILTER_LAYER_TOKENS>(symbol, message.token);
    if (result != Token_Trie::Result::First_Insertion)
      return;

    const auto sft = shared_from_this();
    const auto &job_queue = message.get_Job_Queue();

    for (auto &output : snapshot.snapshot<FILTER_LAYER_OUTPUTS>())
      job_queue->give_one(std::make_shared<Message_Token_Insert>(output, message.network, sft, Node_Key_Symbol_2::Create(symbol), message.token));
  }

  void Node_Filter_2::receive(const Message_Token_Remove &message) {
    const auto &wme = *std::dynamic_pointer_cast<const Token_Alpha>(message.token)->get_wme();
    const auto &symbols = wme.get_symbols();
    const auto &symbol = std::get<2>(wme.get_symbols());

    const auto[result, snapshot, value] = m_filter_layer_trie.erase<FILTER_LAYER_TOKENS>(symbol, message.token);
    if (result != Token_Trie::Result::Last_Removal)
      return;

    const auto sft = shared_from_this();
    const auto &job_queue = message.get_Job_Queue();

    for (auto &output : snapshot.snapshot<FILTER_LAYER_OUTPUTS>())
      job_queue->give_one(std::make_shared<Message_Token_Remove>(output, message.network, sft, Node_Key_Symbol_2::Create(symbol), message.token));
  }

  void Node_Filter_2::receive(const Message_Connect_Filter_2 &message) {
    insert_tokens(message.network, message.get_Job_Queue(), message.key, message.child, message.snapshot);
  }

  void Node_Filter_2::receive(const Message_Disconnect_Output &message) {
    const auto key_node = message.key;
    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol_2>(key_node);
    assert(key_symbol);

    const auto[result, snapshot, value] = m_filter_layer_trie.erase_ip_xp<FILTER_LAYER_OUTPUTS, FILTER_LAYER_OUTPUTS_UNLINKED>(key_symbol->symbol, message.child);

    assert(result != Node_Trie::Result::Failed_Removal);

    if (result != Node_Trie::Result::Last_Removal_IP && result != Node_Trie::Result::Last_Removal)
      return;

    send_disconnect_from_parents(message.network, message.get_Job_Queue());

    if (result != Node_Trie::Result::Last_Removal_IP)
      return;

    remove_tokens(message.network, message.get_Job_Queue(), message.key, message.child, snapshot);
  }

  void Node_Filter_2::insert_tokens(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child, const Filter_Layer_Snapshot snapshot) {
    const auto sft = shared_from_this();

    const auto key_node = key;
    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol_2>(key_node);
    assert(key_symbol);

    for (const auto &token : snapshot.snapshot<FILTER_LAYER_TOKENS>())
      job_queue->give_one(std::make_shared<Message_Token_Insert>(child, network, sft, Node_Key_Symbol_2::Create(key_symbol->symbol), token));
  }

  void Node_Filter_2::remove_tokens(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child, const Filter_Layer_Snapshot snapshot) {
    const auto sft = shared_from_this();

    const auto key_node = key;
    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol_2>(key_node);
    assert(key_symbol);

    for (const auto &token : snapshot.snapshot<FILTER_LAYER_TOKENS>())
      job_queue->give_one(std::make_shared<Message_Token_Remove>(child, network, sft, Node_Key_Symbol_2::Create(key_symbol->symbol), token));
  }

  bool Node_Filter_2::operator==(const Node &rhs) const {
    //return this == &rhs;

    if (auto filter_2 = dynamic_cast<const Node_Filter_2 *>(&rhs)) {
      return *get_key() == *filter_2->get_key() && get_input() == filter_2->get_input();
    }

    return false;
  }

}
