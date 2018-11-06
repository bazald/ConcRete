#include "Zeni/Rete/Node_Filter_0.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Internal/Debug_Counters.hpp"
#include "Zeni/Rete/Internal/Message_Disconnect_Output.hpp"
#include "Zeni/Rete/Internal/Message_Token_Insert.hpp"
#include "Zeni/Rete/Internal/Message_Token_Remove.hpp"
#include "Zeni/Rete/Internal/Token_Alpha.hpp"
#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Action.hpp"

#include <cassert>

namespace Zeni::Rete {

  Node_Filter_0::Node_Filter_0(const std::shared_ptr<Network> network, const std::shared_ptr<const Symbol> &symbol)
    : Node_Unary(1, 1, 1, hash_combine(std::hash<int>()(1), symbol->hash()), network),
    m_symbol(symbol)
  {
  }

  std::shared_ptr<const Symbol> Node_Filter_0::get_symbol() const {
    return m_symbol;
  }

  std::shared_ptr<Node_Filter_0> Node_Filter_0::Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Symbol> &symbol) {
    class Friendly_Node_Filter_0 : public Node_Filter_0 {
    public:
      Friendly_Node_Filter_0(const std::shared_ptr<Network> &network, const std::shared_ptr<const Symbol> &symbol) : Node_Filter_0(network, symbol) {}
    };

    const auto created = std::shared_ptr<Friendly_Node_Filter_0>(new Friendly_Node_Filter_0(network, symbol));
    const auto [result, connected] = network->connect_new_or_existing_output(network, job_queue, created);

    return std::static_pointer_cast<Node_Filter_0>(connected);
  }

  void Node_Filter_0::receive(const Message_Token_Insert &message) {
    const auto &wme = *std::dynamic_pointer_cast<const Token_Alpha>(message.token)->get_wme();
    const auto &symbol = std::get<0>(wme.get_symbols());

    const auto[result, snapshot, value] = m_filter_layer_1_trie.insert_2<FILTER_LAYER_1_SYMBOL, FILTER_LAYER_1_SYMBOL_TOKENS>(symbol, message.token);
    if (result != Token_Trie::Result::First_Insertion)
      return;

    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    for (auto &output : snapshot.lookup_snapshot<FILTER_LAYER_1_SYMBOL, FILTER_LAYER_1_SYMBOL_OUTPUTS>(symbol))
      jobs.emplace_back(std::make_shared<Message_Token_Insert>(output, message.network, sft, message.token));
    for (auto &output : snapshot.snapshot<FILTER_LAYER_1_VARIABLE_OUTPUTS>())
      jobs.emplace_back(std::make_shared<Message_Token_Insert>(output, message.network, sft, message.token));

    //for (const auto tokens : snapshot.snapshot<FILTER_LAYER_1_SYMBOL>()) {
    //  for (const auto &token : tokens.second.snapshot<FILTER_LAYER_1_SYMBOL_TOKENS>())
    //    token->print(std::cerr);
    //}

    message.get_Job_Queue()->give_many(std::move(jobs));
  }

  void Node_Filter_0::receive(const Message_Token_Remove &message) {
    const auto &wme = *std::dynamic_pointer_cast<const Token_Alpha>(message.token)->get_wme();
    const auto &symbol = std::get<0>(wme.get_symbols());

    const auto[result, snapshot, value] = m_filter_layer_1_trie.erase_2<FILTER_LAYER_1_SYMBOL, FILTER_LAYER_1_SYMBOL_TOKENS>(symbol, message.token);
    if (result != Token_Trie::Result::Last_Removal)
      return;

    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    for (auto &output : snapshot.lookup_snapshot<FILTER_LAYER_1_SYMBOL, FILTER_LAYER_1_SYMBOL_OUTPUTS>(symbol))
      jobs.emplace_back(std::make_shared<Message_Token_Remove>(output, message.network, sft, message.token));
    for (auto &output : snapshot.snapshot<FILTER_LAYER_1_VARIABLE_OUTPUTS>())
      jobs.emplace_back(std::make_shared<Message_Token_Remove>(output, message.network, sft, message.token));

    message.get_Job_Queue()->give_many(std::move(jobs));
  }

  bool Node_Filter_0::operator==(const Node &rhs) const {
    //return this == &rhs;

    if (auto filter_0 = dynamic_cast<const Node_Filter_0 *>(&rhs)) {
      return *m_symbol == *filter_0->m_symbol;
    }

    return false;
  }

}
