#include "Zeni/Rete/Node_Filter_1.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Internal/Debug_Counters.hpp"
#include "Zeni/Rete/Internal/Message_Connect_Filter_1.hpp"
#include "Zeni/Rete/Internal/Message_Disconnect_Output.hpp"
#include "Zeni/Rete/Internal/Message_Token_Insert.hpp"
#include "Zeni/Rete/Internal/Message_Token_Remove.hpp"
#include "Zeni/Rete/Internal/Node_Key.hpp"
#include "Zeni/Rete/Internal/Token_Alpha.hpp"
#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Action.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Filter_1::Node_Filter_1(const std::shared_ptr<Network> network, const std::shared_ptr<const Node_Key> node_key)
    : Node_Unary(1, 1, 1, hash_combine(std::hash<int>()(1), node_key->hash()), node_key, network)
  {
    assert(dynamic_cast<const Node_Key_Symbol *>(node_key.get())
      || dynamic_cast<const Node_Key_Null *>(node_key.get()));
  }

  std::shared_ptr<Node_Filter_1> Node_Filter_1::Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> node_key) {
    class Friendly_Node_Filter_0 : public Node_Filter_1 {
    public:
      Friendly_Node_Filter_0(const std::shared_ptr<Network> &network, const std::shared_ptr<const Node_Key> node_key) : Node_Filter_1(network, node_key) {}
    };

    const auto created = std::shared_ptr<Friendly_Node_Filter_0>(new Friendly_Node_Filter_0(network, node_key));
    const auto [result, connected] = network->connect_new_or_existing_output(network, job_queue, created);

    return std::static_pointer_cast<Node_Filter_1>(connected);
  }

  std::pair<Node::Node_Trie::Result, std::shared_ptr<Node>> Node_Filter_1::connect_new_or_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> child) {
    const auto sft = shared_from_this();

    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(child->get_key_for_input(sft));
    assert(key_symbol || dynamic_cast<const Node_Key_12 *>(child->get_key_for_input(sft).get()));

    const auto[result, snapshot, value] = key_symbol
      ? m_filter_layer_1_trie.insert_2<FILTER_LAYER_1_SYMBOL, FILTER_LAYER_1_SYMBOL_OUTPUTS>(key_symbol->symbol, child)
      : m_filter_layer_1_trie.insert<FILTER_LAYER_1_VARIABLE_OUTPUTS_01>(child);

    if (result == Node_Trie::Result::First_Insertion)
      job_queue->give_one(std::make_shared<Message_Connect_Filter_1>(sft, network, std::move(snapshot), value));
    else
      DEBUG_COUNTER_DECREMENT(g_node_increments, 1);

    return std::make_pair(result, value);
  }

  Node::Node_Trie::Result Node_Filter_1::connect_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> child) {
    const auto sft = shared_from_this();

    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(child->get_key_for_input(sft));
    assert(key_symbol || dynamic_cast<const Node_Key_12 *>(child->get_key_for_input(sft).get()));

    const auto[result, snapshot, value] = key_symbol
      ? m_filter_layer_1_trie.insert_2<FILTER_LAYER_1_SYMBOL, FILTER_LAYER_1_SYMBOL_OUTPUTS>(key_symbol->symbol, child)
      : m_filter_layer_1_trie.insert<FILTER_LAYER_1_VARIABLE_OUTPUTS_01>(child);

    assert(value == child);

    if (result == Node_Trie::Result::First_Insertion)
      job_queue->give_one(std::make_shared<Message_Connect_Filter_1>(sft, network, std::move(snapshot), value));

    return result;
  }

  void Node_Filter_1::receive(const Message_Token_Insert &message) {
    const auto &wme = *std::dynamic_pointer_cast<const Token_Alpha>(message.token)->get_wme();
    const auto &symbols = wme.get_symbols();
    const auto &symbol = std::get<1>(symbols);

    const auto[result, snapshot, value] = m_filter_layer_1_trie.insert_2<FILTER_LAYER_1_SYMBOL, FILTER_LAYER_1_SYMBOL_TOKENS>(symbol, message.token);
    if (result != Token_Trie::Result::First_Insertion)
      return;

    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    for (auto &output : snapshot.lookup_snapshot<FILTER_LAYER_1_SYMBOL, FILTER_LAYER_1_SYMBOL_OUTPUTS>(symbol))
      jobs.emplace_back(std::make_shared<Message_Token_Insert>(output, message.network, sft, message.token));
    if (*std::get<0>(symbols) == *std::get<1>(symbols)) {
      for (auto &output : snapshot.snapshot<FILTER_LAYER_1_VARIABLE_OUTPUTS_01>())
        jobs.emplace_back(std::make_shared<Message_Token_Insert>(output, message.network, sft, message.token));
    }

    message.get_Job_Queue()->give_many(std::move(jobs));
  }

  void Node_Filter_1::receive(const Message_Token_Remove &message) {
    const auto &wme = *std::dynamic_pointer_cast<const Token_Alpha>(message.token)->get_wme();
    const auto &symbols = wme.get_symbols();
    const auto &symbol = std::get<1>(wme.get_symbols());

    const auto[result, snapshot, value] = m_filter_layer_1_trie.erase_2<FILTER_LAYER_1_SYMBOL, FILTER_LAYER_1_SYMBOL_TOKENS>(symbol, message.token);
    if (result != Token_Trie::Result::Last_Removal)
      return;

    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    for (auto &output : snapshot.lookup_snapshot<FILTER_LAYER_1_SYMBOL, FILTER_LAYER_1_SYMBOL_OUTPUTS>(symbol))
      jobs.emplace_back(std::make_shared<Message_Token_Remove>(output, message.network, sft, message.token));
    if (*std::get<0>(symbols) == *std::get<1>(symbols)) {
      for (auto &output : snapshot.snapshot<FILTER_LAYER_1_VARIABLE_OUTPUTS_01>())
        jobs.emplace_back(std::make_shared<Message_Token_Remove>(output, message.network, sft, message.token));
    }

    message.get_Job_Queue()->give_many(std::move(jobs));
  }

  void Node_Filter_1::receive(const Message_Connect_Filter_1 &message) {
    const auto sft = shared_from_this();

    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(message.child->get_key_for_input(sft));

    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    if (key_symbol) {
      for (const auto &token : message.snapshot.lookup_snapshot<FILTER_LAYER_1_SYMBOL, FILTER_LAYER_1_SYMBOL_TOKENS>(key_symbol->symbol))
        jobs.emplace_back(std::make_shared<Message_Token_Insert>(message.child, message.network, sft, token));
    }
    else {
      for (const auto tokens : message.snapshot.snapshot<FILTER_LAYER_1_SYMBOL>()) {
        for (const auto &token : tokens.second.snapshot<FILTER_LAYER_1_SYMBOL_TOKENS>()) {
          const auto token_alpha = std::dynamic_pointer_cast<const Token_Alpha>(token);
          const auto &symbols = token_alpha->get_wme()->get_symbols();
          if (*std::get<0>(symbols) == *std::get<1>(symbols))
            jobs.emplace_back(std::make_shared<Message_Token_Insert>(message.child, message.network, sft, token));
        }
      }
    }

    message.get_Job_Queue()->give_many(std::move(jobs));
  }

  void Node_Filter_1::receive(const Message_Disconnect_Output &message) {
    const auto sft = shared_from_this();

    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(message.child->get_key_for_input(sft));

    const auto[result, snapshot, value] = key_symbol
      ? m_filter_layer_1_trie.erase_2<FILTER_LAYER_1_SYMBOL, FILTER_LAYER_1_SYMBOL_OUTPUTS>(key_symbol->symbol, message.child)
      : m_filter_layer_1_trie.erase<FILTER_LAYER_1_VARIABLE_OUTPUTS_01>(message.child);

    if (result != Node_Trie::Result::Last_Removal)
      return;

    send_disconnect_from_parents(message.network, message.get_Job_Queue());

    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    if (key_symbol) {
      for (const auto &token : snapshot.lookup_snapshot<FILTER_LAYER_1_SYMBOL, FILTER_LAYER_1_SYMBOL_TOKENS>(key_symbol->symbol))
        jobs.emplace_back(std::make_shared<Message_Token_Remove>(message.child, message.network, sft, token));
    }
    else {
      for (const auto tokens : snapshot.snapshot<FILTER_LAYER_1_SYMBOL>()) {
        for (const auto &token : tokens.second.snapshot<FILTER_LAYER_1_SYMBOL_TOKENS>()) {
          const auto token_alpha = std::dynamic_pointer_cast<const Token_Alpha>(token);
          const auto &symbols = token_alpha->get_wme()->get_symbols();
          if (*std::get<0>(symbols) == *std::get<1>(symbols))
            jobs.emplace_back(std::make_shared<Message_Token_Remove>(message.child, message.network, sft, token));
        }
      }
    }

    message.get_Job_Queue()->give_many(std::move(jobs));
  }

  bool Node_Filter_1::operator==(const Node &rhs) const {
    //return this == &rhs;

    if (auto filter_1 = dynamic_cast<const Node_Filter_1 *>(&rhs)) {
      return *get_key() == *filter_1->get_key();
    }

    return false;
  }

}
