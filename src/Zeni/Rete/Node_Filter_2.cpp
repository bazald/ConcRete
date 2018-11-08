#include "Zeni/Rete/Node_Filter_2.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Internal/Debug_Counters.hpp"
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
      || dynamic_cast<const Node_Key_Null *>(node_key.get())
      || dynamic_cast<const Node_Key_12 *>(node_key.get()));
  }

  std::shared_ptr<Node_Filter_2> Node_Filter_2::Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> node_key, const std::shared_ptr<Node> input) {
    class Friendly_Node_Filter_2 : public Node_Filter_2 {
    public:
      Friendly_Node_Filter_2(const std::shared_ptr<Network> &network, const std::shared_ptr<const Node_Key> node_key, const std::shared_ptr<Node> input) : Node_Filter_2(network, node_key, input) {}
    };

    const auto created = std::shared_ptr<Friendly_Node_Filter_2>(new Friendly_Node_Filter_2(network, node_key, input));
    const auto [result, connected] = input->connect_new_or_existing_output(network, job_queue, node_key, created);

    if (result != Node_Trie::Result::First_Insertion)
      input->send_disconnect_from_parents(network, job_queue);

    return std::static_pointer_cast<Node_Filter_2>(connected);
  }

  std::pair<Node::Node_Trie::Result, std::shared_ptr<Node>> Node_Filter_2::connect_new_or_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) {
    const auto sft = shared_from_this();

    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(key);
    assert(key_symbol || dynamic_cast<const Node_Key_02 *>(key.get()) || dynamic_cast<const Node_Key_12 *>(key.get()));

    const auto[result, snapshot, value] = key_symbol
      ? m_filter_layer_2_trie.insert_2<FILTER_LAYER_2_SYMBOL, FILTER_LAYER_2_SYMBOL_OUTPUTS>(key_symbol->symbol, child)
      : dynamic_cast<const Node_Key_02 *>(key.get())
      ? m_filter_layer_2_trie.insert<FILTER_LAYER_2_VARIABLE_OUTPUTS_02>(child)
      : m_filter_layer_2_trie.insert<FILTER_LAYER_2_VARIABLE_OUTPUTS_12>(child);

    if (result == Node_Trie::Result::First_Insertion)
      job_queue->give_one(std::make_shared<Message_Connect_Filter_2>(sft, network, std::move(snapshot), key, value));
    else
      DEBUG_COUNTER_DECREMENT(g_node_increments, 1);

    return std::make_pair(result, value);
  }

  Node::Node_Trie::Result Node_Filter_2::connect_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) {
    const auto sft = shared_from_this();

    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(key);
    assert(key_symbol || dynamic_cast<const Node_Key_02 *>(key.get()) || dynamic_cast<const Node_Key_12 *>(key.get()));

    const auto[result, snapshot, value] = key_symbol
      ? m_filter_layer_2_trie.insert_2<FILTER_LAYER_2_SYMBOL, FILTER_LAYER_2_SYMBOL_OUTPUTS>(key_symbol->symbol, child)
      : dynamic_cast<const Node_Key_02 *>(key.get())
      ? m_filter_layer_2_trie.insert<FILTER_LAYER_2_VARIABLE_OUTPUTS_02>(child)
      : m_filter_layer_2_trie.insert<FILTER_LAYER_2_VARIABLE_OUTPUTS_12>(child);

    assert(value == child);

    if (result == Node_Trie::Result::First_Insertion)
      job_queue->give_one(std::make_shared<Message_Connect_Filter_2>(sft, network, std::move(snapshot), key, value));

    return result;
  }

  void Node_Filter_2::receive(const Message_Token_Insert &message) {
    const auto &wme = *std::dynamic_pointer_cast<const Token_Alpha>(message.token)->get_wme();
    const auto &symbols = wme.get_symbols();
    const auto &symbol = std::get<2>(symbols);

    const auto[result, snapshot, value] = m_filter_layer_2_trie.insert_2<FILTER_LAYER_2_SYMBOL, FILTER_LAYER_2_SYMBOL_TOKENS>(symbol, message.token);
    if (result != Token_Trie::Result::First_Insertion)
      return;

    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    for (auto &output : snapshot.lookup_snapshot<FILTER_LAYER_2_SYMBOL, FILTER_LAYER_2_SYMBOL_OUTPUTS>(symbol))
      jobs.emplace_back(std::make_shared<Message_Token_Insert>(output, message.network, sft, std::make_shared<Node_Key_Symbol>(symbol), message.token));
    if (*std::get<0>(symbols) == *std::get<2>(symbols)) {
      for (auto &output : snapshot.snapshot<FILTER_LAYER_2_VARIABLE_OUTPUTS_02>())
        jobs.emplace_back(std::make_shared<Message_Token_Insert>(output, message.network, sft, std::make_shared<Node_Key_02>(), message.token));
    }
    if (*std::get<1>(symbols) == *std::get<2>(symbols)) {
      for (auto &output : snapshot.snapshot<FILTER_LAYER_2_VARIABLE_OUTPUTS_12>())
        jobs.emplace_back(std::make_shared<Message_Token_Insert>(output, message.network, sft, std::make_shared<Node_Key_12>(), message.token));
    }

    message.get_Job_Queue()->give_many(std::move(jobs));
  }

  void Node_Filter_2::receive(const Message_Token_Remove &message) {
    const auto &wme = *std::dynamic_pointer_cast<const Token_Alpha>(message.token)->get_wme();
    const auto &symbols = wme.get_symbols();
    const auto &symbol = std::get<2>(wme.get_symbols());

    const auto[result, snapshot, value] = m_filter_layer_2_trie.erase_2<FILTER_LAYER_2_SYMBOL, FILTER_LAYER_2_SYMBOL_TOKENS>(symbol, message.token);
    if (result != Token_Trie::Result::Last_Removal)
      return;

    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    for (auto &output : snapshot.lookup_snapshot<FILTER_LAYER_2_SYMBOL, FILTER_LAYER_2_SYMBOL_OUTPUTS>(symbol))
      jobs.emplace_back(std::make_shared<Message_Token_Remove>(output, message.network, sft, std::make_shared<Node_Key_Symbol>(symbol), message.token));
    if (*std::get<0>(symbols) == *std::get<2>(symbols)) {
      for (auto &output : snapshot.snapshot<FILTER_LAYER_2_VARIABLE_OUTPUTS_02>())
        jobs.emplace_back(std::make_shared<Message_Token_Remove>(output, message.network, sft, std::make_shared<Node_Key_02>(), message.token));
    }
    if (*std::get<1>(symbols) == *std::get<2>(symbols)) {
      for (auto &output : snapshot.snapshot<FILTER_LAYER_2_VARIABLE_OUTPUTS_12>())
        jobs.emplace_back(std::make_shared<Message_Token_Remove>(output, message.network, sft, std::make_shared<Node_Key_12>(), message.token));
    }

    message.get_Job_Queue()->give_many(std::move(jobs));
  }

  void Node_Filter_2::receive(const Message_Connect_Filter_2 &message) {
    const auto sft = shared_from_this();

    const auto key_node = message.key;
    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(key_node);
    assert(key_symbol || std::dynamic_pointer_cast<const Node_Key_02>(key_node) || std::dynamic_pointer_cast<const Node_Key_12>(key_node));

    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    if (key_symbol) {
      for (const auto &token : message.snapshot.lookup_snapshot<FILTER_LAYER_2_SYMBOL, FILTER_LAYER_2_SYMBOL_TOKENS>(key_symbol->symbol))
        jobs.emplace_back(std::make_shared<Message_Token_Insert>(message.child, message.network, sft, std::make_shared<Node_Key_Symbol>(key_symbol->symbol), token));
    }
    else if (dynamic_cast<const Node_Key_02 *>(key_node.get())) {
      for (const auto tokens : message.snapshot.snapshot<FILTER_LAYER_2_SYMBOL>()) {
        for (const auto &token : tokens.second.snapshot<FILTER_LAYER_2_SYMBOL_TOKENS>()) {
          const auto token_alpha = std::dynamic_pointer_cast<const Token_Alpha>(token);
          const auto &symbols = token_alpha->get_wme()->get_symbols();
          if (*std::get<0>(symbols) == *std::get<2>(symbols))
            jobs.emplace_back(std::make_shared<Message_Token_Insert>(message.child, message.network, sft, std::make_shared<Node_Key_02>(), token));
        }
      }
    }
    else {
      for (const auto tokens : message.snapshot.snapshot<FILTER_LAYER_2_SYMBOL>()) {
        for (const auto &token : tokens.second.snapshot<FILTER_LAYER_2_SYMBOL_TOKENS>()) {
          const auto token_alpha = std::dynamic_pointer_cast<const Token_Alpha>(token);
          const auto &symbols = token_alpha->get_wme()->get_symbols();
          if (*std::get<1>(symbols) == *std::get<2>(symbols))
            jobs.emplace_back(std::make_shared<Message_Token_Insert>(message.child, message.network, sft, std::make_shared<Node_Key_12>(), token));
        }
      }
    }

    message.get_Job_Queue()->give_many(std::move(jobs));
  }

  void Node_Filter_2::receive(const Message_Disconnect_Output &message) {
    const auto sft = shared_from_this();

    const auto key_node = message.key;
    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(key_node);
    assert(key_symbol || std::dynamic_pointer_cast<const Node_Key_02>(key_node) || std::dynamic_pointer_cast<const Node_Key_12>(key_node));

    const auto[result, snapshot, value] = key_symbol
      ? m_filter_layer_2_trie.erase_2<FILTER_LAYER_2_SYMBOL, FILTER_LAYER_2_SYMBOL_OUTPUTS>(key_symbol->symbol, message.child)
      : dynamic_cast<const Node_Key_02 *>(key_node.get())
      ? m_filter_layer_2_trie.erase<FILTER_LAYER_2_VARIABLE_OUTPUTS_02>(message.child)
      : m_filter_layer_2_trie.erase<FILTER_LAYER_2_VARIABLE_OUTPUTS_12>(message.child);

    if (result != Node_Trie::Result::Last_Removal)
      return;

    send_disconnect_from_parents(message.network, message.get_Job_Queue());

    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    if (key_symbol) {
      for (const auto &token : snapshot.lookup_snapshot<FILTER_LAYER_2_SYMBOL, FILTER_LAYER_2_SYMBOL_TOKENS>(key_symbol->symbol))
        jobs.emplace_back(std::make_shared<Message_Token_Remove>(message.child, message.network, sft, std::make_shared<Node_Key_Symbol>(key_symbol->symbol), token));
    }
    else if (dynamic_cast<const Node_Key_02 *>(key_node.get())) {
      for (const auto tokens : snapshot.snapshot<FILTER_LAYER_2_SYMBOL>()) {
        for (const auto &token : tokens.second.snapshot<FILTER_LAYER_2_SYMBOL_TOKENS>()) {
          const auto token_alpha = std::dynamic_pointer_cast<const Token_Alpha>(token);
          const auto &symbols = token_alpha->get_wme()->get_symbols();
          if (*std::get<0>(symbols) == *std::get<2>(symbols))
            jobs.emplace_back(std::make_shared<Message_Token_Remove>(message.child, message.network, sft, std::make_shared<Node_Key_02>(), token));
        }
      }
    }
    else {
      for (const auto tokens : snapshot.snapshot<FILTER_LAYER_2_SYMBOL>()) {
        for (const auto &token : tokens.second.snapshot<FILTER_LAYER_2_SYMBOL_TOKENS>()) {
          const auto token_alpha = std::dynamic_pointer_cast<const Token_Alpha>(token);
          const auto &symbols = token_alpha->get_wme()->get_symbols();
          if (*std::get<1>(symbols) == *std::get<2>(symbols))
            jobs.emplace_back(std::make_shared<Message_Token_Remove>(message.child, message.network, sft, std::make_shared<Node_Key_12>(), token));
        }
      }
    }

    message.get_Job_Queue()->give_many(std::move(jobs));
  }

  bool Node_Filter_2::operator==(const Node &rhs) const {
    //return this == &rhs;

    if (auto filter_2 = dynamic_cast<const Node_Filter_2 *>(&rhs)) {
      return *get_key() == *filter_2->get_key() && get_input() == filter_2->get_input();
    }

    return false;
  }

}
