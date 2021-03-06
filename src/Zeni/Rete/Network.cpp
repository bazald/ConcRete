#include "Zeni/Rete/Network.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Concurrency/Worker_Threads.hpp"
#include "Zeni/Rete/Internal/Message_Connect_Filter_0.hpp"
#include "Zeni/Rete/Internal/Message_Disconnect_Output.hpp"
#include "Zeni/Rete/Internal/Message_Token_Insert.hpp"
#include "Zeni/Rete/Internal/Message_Token_Remove.hpp"
#include "Zeni/Rete/Internal/Token_Alpha.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Node_Key.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <string_view>

namespace Zeni::Rete {

  std::shared_ptr<const Network> Network::shared_from_this() const {
    return std::static_pointer_cast<const Network>(Concurrency::Recipient::shared_from_this());
  }

  std::shared_ptr<Network> Network::shared_from_this() {
    return std::static_pointer_cast<Network>(Concurrency::Recipient::shared_from_this());
  }

  Network::Instantiation::Instantiation(const std::shared_ptr<Network> network)
    : m_network(network)
  {
  }

  std::shared_ptr<Network::Instantiation> Network::Instantiation::Create(const std::shared_ptr<Network> network) {
    class Friendly_Network_Instantiation : public Instantiation {
    public:
      Friendly_Network_Instantiation(const std::shared_ptr<Network> network)
        : Instantiation(network)
      {
      }
    };

    return std::make_shared<Friendly_Network_Instantiation>(network);
  }

  Network::Instantiation::~Instantiation() {
    m_network->Destroy();
  }

  std::shared_ptr<const Network> Network::Instantiation::get() const {
    return m_network;
  }

  std::shared_ptr<Network> Network::Instantiation::get() {
    return m_network;
  }

  const Network * Network::Instantiation::operator*() const {
    return m_network.get();
  }

  Network * Network::Instantiation::operator*() {
    return m_network.get();
  }

  const Network * Network::Instantiation::operator->() const {
    return m_network.get();
  }

  Network * Network::Instantiation::operator->() {
    return m_network.get();
  }

  Concurrency::Intrusive_Shared_Ptr<Network::Genatom>::Lock Network::Genatom::next() const {
    std::string str = m_str;
    std::string::iterator st = str.begin(), send = str.end();
    do {
      switch (*st) {
      case 'z': *st = '0';
        continue;
      case 'Z': *st = 'a';
        return new Genatom(str);
      case '9': *st = 'A';
        return new Genatom(str);
      default:
        ++*st;
        return new Genatom(str);
      }
    } while(++st != send);
    str += '0';
    return new Genatom(str);
  }

  std::string_view Network::Genatom::str() const {
    return m_str;
  }

  Network::Network(const Network::Printed_Output printed_output)
    : Node(0, 0, 1, 0),
    m_worker_threads(Concurrency::Worker_Threads::Create()),
    m_printed_output(printed_output)
  {
  }

  std::shared_ptr<Network::Instantiation> Network::Create(const Network::Printed_Output printed_output) {
    class Friendly_Network : public Network {
    public:
      Friendly_Network(const Network::Printed_Output &printed_output)
        : Network(printed_output)
      {
      }
    };

    auto network_instantiation = Instantiation::Create(std::shared_ptr<Friendly_Network>(new Friendly_Network(printed_output)));

    return network_instantiation;
  }

  Network::Network(const Printed_Output printed_output, const std::shared_ptr<Concurrency::Worker_Threads> &worker_threads)
    : Node(0, 0, 1, 0),
    m_worker_threads(worker_threads),
    m_printed_output(printed_output)
  {
  }

  std::shared_ptr<Network::Instantiation> Network::Create(const Network::Printed_Output printed_output, const std::shared_ptr<Concurrency::Worker_Threads> worker_threads) {
    class Friendly_Network : public Network {
    public:
      Friendly_Network(const Network::Printed_Output &printed_output, const std::shared_ptr<Concurrency::Worker_Threads> &worker_threads)
        : Network(printed_output, worker_threads)
      {
      }
    };

    return Instantiation::Create(std::shared_ptr<Friendly_Network>(new Friendly_Network(printed_output, worker_threads)));
  }

  void Network::Destroy() {
    if (m_worker_threads) {
      excise_all(m_worker_threads->get_main_Job_Queue(), false);
      m_worker_threads->finish_jobs();
    }
  }

  void Network::connect_to_parents_again(const std::shared_ptr<Network>, const std::shared_ptr<Concurrency::Job_Queue>)
  {
  }

  void Network::send_disconnect_from_parents(const std::shared_ptr<Network>, const std::shared_ptr<Concurrency::Job_Queue>)
  {
  }

  Network::~Network()
  {
  }

  std::shared_ptr<Concurrency::Worker_Threads> Network::get_Worker_Threads() const {
    return m_worker_threads;
  }

  std::shared_ptr<Node_Action> Network::get_rule(const std::string &name) const {
    const auto [result, found] = m_rules.lookup(name);
    return found;
  }

  std::set<std::string> Network::get_rule_names() const {
    const auto rules = m_rules.snapshot();
    std::set<std::string> rv;
    for (auto rule : *rules)
      rv.emplace(rule->get_name());
    return rv;
  }

  void Network::init_genatom(const std::string &str) {
    m_genatom.store(new Genatom(str), std::memory_order_release);
  }

  std::string Network::get_genatom() const {
    const auto genatom = m_genatom.load(std::memory_order_acquire);
    return std::string(genatom->str());
  }

  std::string Network::genatom() {
    auto current = m_genatom.load();
    auto next = current->next();
    while (!m_genatom.compare_exchange_strong(current, next, std::memory_order_release, std::memory_order_acquire))
      next = current->next();
    return std::string(current->str());
  }

  bool Network::is_exit_requested() const {
    return m_exit_requested.load();
  }

  void Network::request_exit() {
    m_exit_requested.store(true);
  }

  Network::Printed_Output Network::get_Printed_Output() const {
    return m_printed_output;
  }

  void Network::finish_jobs() {
    m_worker_threads->finish_jobs();
  }

  void Network::finish_jobs_and_destroy_worker_threads() {
    finish_jobs();
    m_worker_threads.reset();
  }

  void Network::set_worker_threads(const std::shared_ptr<Concurrency::Worker_Threads> worker_threads) {
    if (m_worker_threads)
      throw Invalid_Worker_Threads_Setup();
    m_worker_threads = worker_threads;
  }

  void Network::excise_all(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action) {
    const auto rules = m_rules.snapshot();
    for (auto rule : *rules)
      excise_rule(job_queue, rule->get_name(), user_action);
  }

  std::pair<Node::Node_Trie::Result, std::shared_ptr<Node>> Network::connect_new_or_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) {
    const auto sft = shared_from_this();
    assert(sft == network);

    const auto key_symbol_0 = std::dynamic_pointer_cast<const Node_Key_Symbol_0>(key);
    const auto key_symbol_1 = std::dynamic_pointer_cast<const Node_Key_Symbol_1>(key);
    const auto key_symbol_2 = std::dynamic_pointer_cast<const Node_Key_Symbol_2>(key);
    assert(key_symbol_0 || key_symbol_1 || key_symbol_2);

    const auto[result, snapshot, value] = key_symbol_0
      ? m_filter_layer_0_trie.insert_ip_xp<FILTER_LAYER_OUTPUTS_UNLINKED, FILTER_LAYER_OUTPUTS>(key_symbol_0->symbol, child)
      : key_symbol_1
      ? m_filter_layer_1_trie.insert_ip_xp<FILTER_LAYER_OUTPUTS_UNLINKED, FILTER_LAYER_OUTPUTS>(key_symbol_1->symbol, child)
      : m_filter_layer_2_trie.insert_ip_xp<FILTER_LAYER_OUTPUTS_UNLINKED, FILTER_LAYER_OUTPUTS>(key_symbol_2->symbol, child);

    if (result == Node_Trie::Result::First_Insertion)
      job_queue->give_one(std::make_shared<Message_Connect_Filter_0>(sft, network, std::move(snapshot), key, value));

    return std::make_pair(result, value);
  }

  Node::Node_Trie::Result Network::connect_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child, const bool unlinked) {
    const auto sft = shared_from_this();
    assert(sft == network);

    const auto key_symbol_0 = std::dynamic_pointer_cast<const Node_Key_Symbol_0>(key);
    const auto key_symbol_1 = std::dynamic_pointer_cast<const Node_Key_Symbol_1>(key);
    const auto key_symbol_2 = std::dynamic_pointer_cast<const Node_Key_Symbol_2>(key);
    assert(key_symbol_0 || key_symbol_1 || key_symbol_2);

    const auto[result, snapshot, value] = unlinked
      ? (key_symbol_0
        ? m_filter_layer_0_trie.insert_ip_xp<FILTER_LAYER_OUTPUTS, FILTER_LAYER_OUTPUTS_UNLINKED>(key_symbol_0->symbol, child)
        : key_symbol_1
        ? m_filter_layer_1_trie.insert_ip_xp<FILTER_LAYER_OUTPUTS, FILTER_LAYER_OUTPUTS_UNLINKED>(key_symbol_1->symbol, child)
        : m_filter_layer_2_trie.insert_ip_xp<FILTER_LAYER_OUTPUTS, FILTER_LAYER_OUTPUTS_UNLINKED>(key_symbol_2->symbol, child))
      : (key_symbol_0
        ? m_filter_layer_0_trie.insert_ip_xp<FILTER_LAYER_OUTPUTS_UNLINKED, FILTER_LAYER_OUTPUTS>(key_symbol_0->symbol, child)
        : key_symbol_1
        ? m_filter_layer_1_trie.insert_ip_xp<FILTER_LAYER_OUTPUTS_UNLINKED, FILTER_LAYER_OUTPUTS>(key_symbol_1->symbol, child)
        : m_filter_layer_2_trie.insert_ip_xp<FILTER_LAYER_OUTPUTS_UNLINKED, FILTER_LAYER_OUTPUTS>(key_symbol_2->symbol, child));

    assert(value == child);

    if (result == Node_Trie::Result::First_Insertion) {
      if (unlinked) {
        if (child->is_linked(sft, key))
          link_output(network, job_queue, key, child);
      }
      else
        job_queue->give_one(std::make_shared<Message_Connect_Filter_0>(sft, network, std::move(snapshot), key, value));
    }

    return result;
  }

  Node::Node_Trie::Result Network::link_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) {
    const auto sft = shared_from_this();

    const auto key_symbol_0 = std::dynamic_pointer_cast<const Node_Key_Symbol_0>(key);
    const auto key_symbol_1 = std::dynamic_pointer_cast<const Node_Key_Symbol_1>(key);
    const auto key_symbol_2 = std::dynamic_pointer_cast<const Node_Key_Symbol_2>(key);
    assert(key_symbol_0 || key_symbol_1 || key_symbol_2);

    const auto[result, snapshot, value] = key_symbol_0
      ? m_filter_layer_0_trie.move<FILTER_LAYER_OUTPUTS_UNLINKED, FILTER_LAYER_OUTPUTS>(key_symbol_0->symbol, child)
      : key_symbol_1
      ? m_filter_layer_1_trie.move<FILTER_LAYER_OUTPUTS_UNLINKED, FILTER_LAYER_OUTPUTS>(key_symbol_1->symbol, child)
      : m_filter_layer_2_trie.move<FILTER_LAYER_OUTPUTS_UNLINKED, FILTER_LAYER_OUTPUTS>(key_symbol_2->symbol, child);

    if (result != Node_Trie::Result::Successful_Move)
      return result;

    insert_tokens(job_queue, key, child, snapshot);

    return result;
  }

  Node::Node_Trie::Result Network::unlink_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) {
    const auto sft = shared_from_this();

    const auto key_symbol_0 = std::dynamic_pointer_cast<const Node_Key_Symbol_0>(key);
    const auto key_symbol_1 = std::dynamic_pointer_cast<const Node_Key_Symbol_1>(key);
    const auto key_symbol_2 = std::dynamic_pointer_cast<const Node_Key_Symbol_2>(key);
    assert(key_symbol_0 || key_symbol_1 || key_symbol_2);

    const auto[result, snapshot, value] = key_symbol_0
      ? m_filter_layer_0_trie.move<FILTER_LAYER_OUTPUTS, FILTER_LAYER_OUTPUTS_UNLINKED>(key_symbol_0->symbol, child)
      : key_symbol_1
      ? m_filter_layer_1_trie.move<FILTER_LAYER_OUTPUTS, FILTER_LAYER_OUTPUTS_UNLINKED>(key_symbol_1->symbol, child)
      : m_filter_layer_2_trie.move<FILTER_LAYER_OUTPUTS, FILTER_LAYER_OUTPUTS_UNLINKED>(key_symbol_2->symbol, child);

    if (result != Node_Trie::Result::Successful_Move)
      return result;

    remove_tokens(job_queue, key, child, snapshot);

    return result;
  }

  bool Network::has_tokens(const std::shared_ptr<const Node_Key> key) const {
    const auto key_symbol_0 = std::dynamic_pointer_cast<const Node_Key_Symbol_0>(key);
    const auto key_symbol_1 = std::dynamic_pointer_cast<const Node_Key_Symbol_1>(key);
    const auto key_symbol_2 = std::dynamic_pointer_cast<const Node_Key_Symbol_2>(key);
    assert(key_symbol_0 || key_symbol_1 || key_symbol_2);

    const auto [result, tokens] = key_symbol_0
      ? m_filter_layer_0_trie.snapshot<FILTER_LAYER_TOKENS>(key_symbol_0->symbol)
      : key_symbol_1
      ? m_filter_layer_1_trie.snapshot<FILTER_LAYER_TOKENS>(key_symbol_1->symbol)
      : m_filter_layer_2_trie.snapshot<FILTER_LAYER_TOKENS>(key_symbol_2->symbol);

    return !tokens.size_zero();
  }

  void Network::receive(const Message_Token_Insert &) {
    abort();
  }

  void Network::receive(const Message_Token_Remove &) {
    abort();
  }

  void Network::receive(const Message_Connect_Filter_0 &message) {
    insert_tokens(message.get_Job_Queue(), message.key, message.child, message.snapshot);
  }

  void Network::receive(const Message_Disconnect_Output &message) {
    const auto key_symbol_0 = std::dynamic_pointer_cast<const Node_Key_Symbol_0>(message.key);
    const auto key_symbol_1 = std::dynamic_pointer_cast<const Node_Key_Symbol_1>(message.key);
    const auto key_symbol_2 = std::dynamic_pointer_cast<const Node_Key_Symbol_2>(message.key);
    assert(key_symbol_0 || key_symbol_1 || key_symbol_2);

    const auto[result, snapshot, value] = key_symbol_0
      ? m_filter_layer_0_trie.erase_ip_xp<FILTER_LAYER_OUTPUTS, FILTER_LAYER_OUTPUTS_UNLINKED>(key_symbol_0->symbol, message.child)
      : key_symbol_1
      ? m_filter_layer_1_trie.erase_ip_xp<FILTER_LAYER_OUTPUTS, FILTER_LAYER_OUTPUTS_UNLINKED>(key_symbol_1->symbol, message.child)
      : m_filter_layer_2_trie.erase_ip_xp<FILTER_LAYER_OUTPUTS, FILTER_LAYER_OUTPUTS_UNLINKED>(key_symbol_2->symbol, message.child);

    assert(result != Node_Trie::Result::Failed_Removal);

    if (result != Node_Trie::Result::Last_Removal_IP)
      return;

    remove_tokens(message.get_Job_Queue(), message.key, message.child, snapshot);
  }

  void Network::insert_tokens(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child, const Filter_Layer_Snapshot snapshot) {
    const auto sft = shared_from_this();

    for (const auto &token : snapshot.snapshot<FILTER_LAYER_TOKENS>())
      job_queue->give_one(std::make_shared<Message_Token_Insert>(child, sft, sft, key, token));
  }

  void Network::remove_tokens(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child, const Filter_Layer_Snapshot snapshot) {
    const auto sft = shared_from_this();

    for (const auto &token : snapshot.snapshot<FILTER_LAYER_TOKENS>())
      job_queue->give_one(std::make_shared<Message_Token_Remove>(child, sft, sft, key, token));
  }

  bool Network::is_linked(const std::shared_ptr<Node>, const std::shared_ptr<const Node_Key>) {
    abort();
  }

  void Network::excise_rule(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::string &name, const bool user_action) {
    auto erased = unname_rule(name, user_action);

    if (erased)
      erased->Destroy(shared_from_this(), job_queue);
  }

  std::string Network::next_rule_name(const std::string_view prefix) {
    std::ostringstream oss;
    for(;;) {
      oss.str("");
      oss << prefix << genatom();
      const auto [result, found] = m_rules.lookup(oss.str());
      if (!found)
        break;
    }
    return oss.str();
  }

  std::shared_ptr<Node_Action> Network::unname_rule(const std::string &name, const bool user_action) {
    const auto[result, erased] = m_rules.erase(std::make_shared<Node_Action>(name));

    if (result == Rule_Trie::Result::Failed_Removal)
      return nullptr;

    if (user_action && m_printed_output != Printed_Output::None)
      std::cerr << '#';

    return erased;
  }

  void Network::insert_wme(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const WME> wme) {
    const auto sft = shared_from_this();
    const auto token = std::make_shared<Token_Alpha>(wme);

    const auto[result0, snapshot0, value0] = m_filter_layer_0_trie.insert<FILTER_LAYER_TOKENS>(std::get<0>(wme->get_symbols()), token);
    if (result0 == Token_Trie::Result::First_Insertion) {
      const auto key = Node_Key_Symbol_0::Create(std::get<0>(wme->get_symbols()));
      for (auto &output : snapshot0.snapshot<FILTER_LAYER_OUTPUTS>())
        job_queue->give_one(std::make_shared<Message_Token_Insert>(output, sft, sft, key, value0));
    }

    const auto[result1, snapshot1, value1] = m_filter_layer_1_trie.insert<FILTER_LAYER_TOKENS>(std::get<1>(wme->get_symbols()), token);
    if (result1 == Token_Trie::Result::First_Insertion) {
      const auto key = Node_Key_Symbol_1::Create(std::get<1>(wme->get_symbols()));
      for (auto &output : snapshot1.snapshot<FILTER_LAYER_OUTPUTS>())
        job_queue->give_one(std::make_shared<Message_Token_Insert>(output, sft, sft, key, value1));
    }

    const auto[result2, snapshot2, value2] = m_filter_layer_2_trie.insert<FILTER_LAYER_TOKENS>(std::get<2>(wme->get_symbols()), token);
    if (result2 == Token_Trie::Result::First_Insertion) {
      const auto key = Node_Key_Symbol_2::Create(std::get<2>(wme->get_symbols()));
      for (auto &output : snapshot2.snapshot<FILTER_LAYER_OUTPUTS>())
        job_queue->give_one(std::make_shared<Message_Token_Insert>(output, sft, sft, key, value2));
    }
  }

  void Network::remove_wme(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const WME> wme) {
    const auto sft = shared_from_this();
    const auto token = std::make_shared<Token_Alpha>(wme);

    const auto[result0, snapshot0, value0] = m_filter_layer_0_trie.insert<FILTER_LAYER_TOKENS>(std::get<0>(wme->get_symbols()), token);
    if (result0 == Token_Trie::Result::Last_Removal) {
      const auto key = Node_Key_Symbol_0::Create(std::get<0>(wme->get_symbols()));
      for (auto &output : snapshot0.snapshot<FILTER_LAYER_OUTPUTS>())
        job_queue->give_one(std::make_shared<Message_Token_Remove>(output, sft, sft, key, value0));
    }

    const auto[result1, snapshot1, value1] = m_filter_layer_1_trie.insert<FILTER_LAYER_TOKENS>(std::get<1>(wme->get_symbols()), token);
    if (result1 == Token_Trie::Result::Last_Removal) {
      const auto key = Node_Key_Symbol_1::Create(std::get<1>(wme->get_symbols()));
      for (auto &output : snapshot1.snapshot<FILTER_LAYER_OUTPUTS>())
        job_queue->give_one(std::make_shared<Message_Token_Remove>(output, sft, sft, key, value1));
    }

    const auto[result2, snapshot2, value2] = m_filter_layer_2_trie.insert<FILTER_LAYER_TOKENS>(std::get<2>(wme->get_symbols()), token);
    if (result2 == Token_Trie::Result::Last_Removal) {
      const auto key = Node_Key_Symbol_2::Create(std::get<2>(wme->get_symbols()));
      for (auto &output : snapshot2.snapshot<FILTER_LAYER_OUTPUTS>())
        job_queue->give_one(std::make_shared<Message_Token_Remove>(output, sft, sft, key, value2));
    }
  }

  void Network::clear_wmes(const std::shared_ptr<Concurrency::Job_Queue> job_queue) {
    /// TODO
    abort();

    //for (const auto tokens : m_filter_layer_0_trie.snapshot<FILTER_LAYER_0_SYMBOL>()) {
    //  for (const auto &token : tokens.second.snapshot<FILTER_LAYER_0_SYMBOL_TOKENS>())
    //    remove_wme(job_queue, dynamic_cast<const Token_Alpha &>(*token).get_wme());
    //}
  }

  void Network::source_rule(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node_Action> action, const bool user_action) {
    const auto[result, inserted, replaced] = m_rules.insert(action);

    if (user_action && m_printed_output != Printed_Output::None) {
      if (replaced)
        std::cerr << '#';
      std::cerr << '*';
    }

    if (replaced)
      replaced->Destroy(shared_from_this(), job_queue);
  }

  bool Network::operator==(const Node &rhs) const {
    return &rhs == this;
  }

}
