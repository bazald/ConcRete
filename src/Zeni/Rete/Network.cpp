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
    const auto [found, snapshot] = m_rules.lookup(name);
    return found;
  }

  std::set<std::string> Network::get_rule_names() const {
    std::set<std::string> rv;
    for (auto rule : m_rules.snapshot())
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
    for (auto rule : m_rules.snapshot())
      excise_rule(job_queue, rule->get_name(), user_action);
  }

  std::pair<Node::Node_Trie::Result, std::shared_ptr<Node>> Network::connect_new_or_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) {
    const auto sft = shared_from_this();
    assert(sft == network);

    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(key);

    const auto[result, snapshot, value] = key_symbol
      ? m_filter_layer_0_trie.insert_2<FILTER_LAYER_0_SYMBOL, FILTER_LAYER_0_SYMBOL_OUTPUTS>(key_symbol->symbol, child)
      : m_filter_layer_0_trie.insert<FILTER_LAYER_0_VARIABLE_OUTPUTS>(child);

    if (result == Node_Trie::Result::First_Insertion)
      job_queue->give_one(std::make_shared<Message_Connect_Filter_0>(sft, network, std::move(snapshot), key, value));

    return std::make_pair(result, value);
  }

  Node::Node_Trie::Result Network::connect_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) {
    const auto sft = shared_from_this();
    assert(sft == network);

    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(key);

    const auto[result, snapshot, value] = key_symbol
      ? m_filter_layer_0_trie.insert_2<FILTER_LAYER_0_SYMBOL, FILTER_LAYER_0_SYMBOL_OUTPUTS>(key_symbol->symbol, child)
      : m_filter_layer_0_trie.insert<FILTER_LAYER_0_VARIABLE_OUTPUTS>(child);

    assert(value == child);

    if (result == Node_Trie::Result::First_Insertion)
      job_queue->give_one(std::make_shared<Message_Connect_Filter_0>(sft, network, std::move(snapshot), key, value));

    return result;
  }

  bool Network::has_tokens(const std::shared_ptr<const Node_Key> key) const {
    if (const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(key)) {
      const auto tokens = m_filter_layer_0_trie.lookup_snapshot<FILTER_LAYER_0_SYMBOL, FILTER_LAYER_0_SYMBOL_TOKENS>(key_symbol->symbol);
      return !tokens.size_zero();
    }
    else {
      assert(dynamic_cast<const Node_Key_Null *>(key.get()));

      const auto snapshot = m_filter_layer_0_trie.snapshot();
      for (const auto tokens : snapshot.snapshot<FILTER_LAYER_0_SYMBOL>()) {
        if (!tokens.second.size_zero<FILTER_LAYER_0_SYMBOL_TOKENS>())
          return true;
      }
      return false;
    }
  }

  void Network::receive(const Message_Token_Insert &) {
    abort();
  }

  void Network::receive(const Message_Token_Remove &) {
    abort();
  }

  void Network::receive(const Message_Connect_Filter_0 &message) {
    const auto sft = shared_from_this();

    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(message.key);

    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    if (key_symbol) {
      for (const auto &token : message.snapshot.lookup_snapshot<FILTER_LAYER_0_SYMBOL, FILTER_LAYER_0_SYMBOL_TOKENS>(key_symbol->symbol))
        jobs.emplace_back(std::make_shared<Message_Token_Insert>(message.child, message.network, sft, Node_Key_Symbol::Create(key_symbol->symbol), token));
    }
    else {
      for (const auto tokens : message.snapshot.snapshot<FILTER_LAYER_0_SYMBOL>()) {
        for (const auto &token : tokens.second.snapshot<FILTER_LAYER_0_SYMBOL_TOKENS>())
          jobs.emplace_back(std::make_shared<Message_Token_Insert>(message.child, message.network, sft, Node_Key_Null::Create(), token));
      }
    }

    message.get_Job_Queue()->give_many(std::move(jobs));
  }

  void Network::receive(const Message_Disconnect_Output &message) {
    const auto sft = shared_from_this();

    const auto key_symbol = std::dynamic_pointer_cast<const Node_Key_Symbol>(message.key);

    const auto[result, snapshot, value] = key_symbol
      ? m_filter_layer_0_trie.erase_2<FILTER_LAYER_0_SYMBOL, FILTER_LAYER_0_SYMBOL_OUTPUTS>(key_symbol->symbol, message.child)
      : m_filter_layer_0_trie.erase<FILTER_LAYER_0_VARIABLE_OUTPUTS>(message.child);

    assert(result != Node_Trie::Result::Failed_Removal);

    if (result != Node_Trie::Result::Last_Removal)
      return;

    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    if (key_symbol) {
      for (const auto &token : snapshot.lookup_snapshot<FILTER_LAYER_0_SYMBOL, FILTER_LAYER_0_SYMBOL_TOKENS>(key_symbol->symbol))
        jobs.emplace_back(std::make_shared<Message_Token_Remove>(message.child, message.network, sft, Node_Key_Symbol::Create(key_symbol->symbol), token));
    }
    else {
      for (const auto tokens : snapshot.snapshot<FILTER_LAYER_0_SYMBOL>()) {
        for (const auto &token : tokens.second.snapshot<FILTER_LAYER_0_SYMBOL_TOKENS>())
          jobs.emplace_back(std::make_shared<Message_Token_Remove>(message.child, message.network, sft, Node_Key_Null::Create(), token));
      }
    }

    message.get_Job_Queue()->give_many(std::move(jobs));
  }

  void Network::excise_rule(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::string &name, const bool user_action) {
    auto erased = unname_rule(name, user_action);

    if(erased)
      erased->Destroy(shared_from_this(), job_queue);
  }

  std::string Network::next_rule_name(const std::string_view prefix) {
    std::ostringstream oss;
    for(;;) {
      oss.str("");
      oss << prefix << genatom();
      const auto [found, snapshot] = m_rules.lookup(oss.str());
      if (!found)
        break;
    }
    return oss.str();
  }

  std::shared_ptr<Node_Action> Network::unname_rule(const std::string &name, const bool user_action) {
    const auto[result, snapshot, erased] = m_rules.erase(std::make_shared<Node_Action>(name));

    if (result == Rule_Trie::Result::Failed_Removal)
      return nullptr;

    if (user_action && m_printed_output != Printed_Output::None)
      std::cerr << '#';

    return erased;
  }

  void Network::insert_wme(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const WME> wme) {
    const auto &symbol = std::get<0>(wme->get_symbols());
    const auto token = std::make_shared<Token_Alpha>(wme);

    const auto[result, snapshot, value] = m_filter_layer_0_trie.insert_2<FILTER_LAYER_0_SYMBOL, FILTER_LAYER_0_SYMBOL_TOKENS>(symbol, token);
    if (result != Token_Trie::Result::First_Insertion)
      return;

    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    for (auto &output : snapshot.lookup_snapshot<FILTER_LAYER_0_SYMBOL, FILTER_LAYER_0_SYMBOL_OUTPUTS>(symbol))
      jobs.emplace_back(std::make_shared<Message_Token_Insert>(output, sft, sft, Node_Key_Symbol::Create(symbol), value));
    for (auto &output : snapshot.snapshot<FILTER_LAYER_0_VARIABLE_OUTPUTS>())
      jobs.emplace_back(std::make_shared<Message_Token_Insert>(output, sft, sft, Node_Key_Null::Create(), value));

    job_queue->give_many(std::move(jobs));
  }

  void Network::remove_wme(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const WME> wme) {
    const auto &symbol = std::get<0>(wme->get_symbols());
    const auto token = std::make_shared<Token_Alpha>(wme);

    const auto[result, snapshot, value] = m_filter_layer_0_trie.erase_2<FILTER_LAYER_0_SYMBOL, FILTER_LAYER_0_SYMBOL_TOKENS>(symbol, token);
    if (result != Token_Trie::Result::Last_Removal)
      return;

    const auto sft = shared_from_this();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    for (auto &output : snapshot.lookup_snapshot<FILTER_LAYER_0_SYMBOL, FILTER_LAYER_0_SYMBOL_OUTPUTS>(symbol))
      jobs.emplace_back(std::make_shared<Message_Token_Remove>(output, sft, sft, Node_Key_Symbol::Create(symbol), value));
    for (auto &output : snapshot.snapshot<FILTER_LAYER_0_VARIABLE_OUTPUTS>())
      jobs.emplace_back(std::make_shared<Message_Token_Remove>(output, sft, sft, Node_Key_Null::Create(), value));

    job_queue->give_many(std::move(jobs));
  }

  void Network::clear_wmes(const std::shared_ptr<Concurrency::Job_Queue> job_queue) {
    for (const auto tokens : m_filter_layer_0_trie.snapshot<FILTER_LAYER_0_SYMBOL>()) {
      for (const auto &token : tokens.second.snapshot<FILTER_LAYER_0_SYMBOL_TOKENS>())
        remove_wme(job_queue, dynamic_cast<const Token_Alpha &>(*token).get_wme());
    }
  }

  void Network::source_rule(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node_Action> action, const bool user_action) {
    const auto[result, snapshot, inserted, replaced] = m_rules.insert(action);

    if (user_action && m_printed_output != Printed_Output::None) {
      if (replaced)
        std::cerr << '#';
      std::cerr << '*';
    }

    if(replaced)
      replaced->Destroy(shared_from_this(), job_queue);
  }

  bool Network::operator==(const Node &rhs) const {
    return &rhs == this;
  }

}
