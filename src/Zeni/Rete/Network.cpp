#include "Zeni/Rete/Network.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Concurrency/Worker_Threads.hpp"
#include "Zeni/Rete/Internal/Antiable_Map.hpp"
#include "Zeni/Rete/Internal/Debug_Counters.hpp"
#include "Zeni/Rete/Internal/Message_Connect_Output.hpp"
#include "Zeni/Rete/Internal/Message_Status_Empty.hpp"
#include "Zeni/Rete/Internal/Message_Status_Nonempty.hpp"
#include "Zeni/Rete/Internal/Message_Token_Insert.hpp"
#include "Zeni/Rete/Internal/Message_Token_Remove.hpp"
#include "Zeni/Rete/Internal/Token_Alpha.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Node_Filter.hpp"

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

  Network::Network(const Network::Printed_Output printed_output)
    : Node(0, 0, 1, 0),
    m_worker_threads(Concurrency::Worker_Threads::Create()),
    m_printed_output(printed_output)
  {
    DEBUG_COUNTER_DECREMENT(g_node_increments, 1);
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
    DEBUG_COUNTER_DECREMENT(g_node_increments, 1);
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
    excise_all(m_worker_threads->get_main_Job_Queue(), false);
    m_worker_threads->finish_jobs();
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
    const auto [found, snapshot] = m_rules.lookup(std::make_shared<Node_Action>(name));
    return found;
  }

  std::set<std::string> Network::get_rule_names() const {
    std::set<std::string> rv;
    for (auto rule : m_rules.snapshot())
      rv.emplace(rule->get_name());
    return rv;
  }

  int64_t Network::get_rule_name_index() const {
    return m_rule_name_index.load();
  }

  void Network::set_rule_name_index(const int64_t rule_name_index_) {
    m_rule_name_index.store(rule_name_index_);
  }

  Network::Printed_Output Network::get_Printed_Output() const {
    return m_printed_output;
  }

  void Network::excise_all(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_command) {
    for (auto rule : m_rules.snapshot())
      excise_rule(job_queue, rule->get_name(), user_command);
  }

  std::pair<std::shared_ptr<Node>, std::shared_ptr<Node>> Network::get_inputs() {
    return std::make_pair(nullptr, nullptr);
  }

  void Network::receive(const Message_Status_Empty &) {
    abort();
  }

  void Network::receive(const Message_Status_Nonempty &) {
    abort();
  }

  void Network::receive(const Message_Token_Insert &) {
    abort();
  }

  void Network::receive(const Message_Token_Remove &) {
    abort();
  }

  void Network::excise_rule(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::string &name, const bool user_command) {
    auto erased = unname_rule(name, user_command);

    if(erased)
      erased->Destroy(shared_from_this(), job_queue);
  }

  std::string Network::next_rule_name(const std::string_view prefix) {
    std::ostringstream oss;
    for(;;) {
      oss.str("");
      oss << prefix << m_rule_name_index.fetch_add() + 1;
      const auto [found, snapshot] = m_rules.lookup(std::make_shared<Node_Action>(oss.str()));
      if (!found)
        break;
    }
    return oss.str();
  }

  std::shared_ptr<Node_Action> Network::unname_rule(const std::string &name, const bool user_command) {
    const auto[result, snapshot, erased] = m_rules.erase(std::make_shared<Node_Action>(name));

    if (result == Rule_Trie::Result::Failed_Removal)
      return nullptr;

    if (user_command && m_printed_output != Printed_Output::None)
      std::cerr << '#';

    return erased;
  }

  void Network::insert_wme(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const WME> wme) {
    const auto[result, snapshot, value] = m_node_data.insert<NODE_DATA_SUBTRIE_TOKEN_OUTPUTS>(std::make_shared<Token_Alpha>(wme));
    if (result != Output_Token_Trie::Result::First_Insertion)
      return;

    const auto sft = shared_from_this();
    const auto output_tokens = snapshot.template snapshot<NODE_DATA_SUBTRIE_TOKEN_OUTPUTS>();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    for (auto &output : snapshot.template snapshot<NODE_DATA_SUBTRIE_OUTPUTS>())
      jobs.emplace_back(std::make_shared<Message_Token_Insert>(output, sft, sft, value));
    if (++output_tokens.cbegin() == output_tokens.cend()) {
      for (auto &output : snapshot.template snapshot<NODE_DATA_SUBTRIE_GATES>())
        jobs.emplace_back(std::make_shared<Message_Status_Nonempty>(output, sft, sft));
    }

    job_queue->give_many(std::move(jobs));
  }

  void Network::remove_wme(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const WME> wme) {
    const auto[result, snapshot, value] = m_node_data.erase<NODE_DATA_SUBTRIE_TOKEN_OUTPUTS>(std::make_shared<Token_Alpha>(wme));
    if (result != Output_Token_Trie::Result::Last_Removal)
      return;

    const auto sft = shared_from_this();
    const auto output_tokens = snapshot.template snapshot<NODE_DATA_SUBTRIE_TOKEN_OUTPUTS>();
    std::vector<std::shared_ptr<Concurrency::IJob>> jobs;

    for (auto &output : snapshot.template snapshot<NODE_DATA_SUBTRIE_OUTPUTS>())
      jobs.emplace_back(std::make_shared<Message_Token_Remove>(output, sft, sft, value));
    if (output_tokens.empty()) {
      for (auto &output : snapshot.template snapshot<NODE_DATA_SUBTRIE_GATES>())
        jobs.emplace_back(std::make_shared<Message_Status_Empty>(output, sft, sft));
    }

    job_queue->give_many(std::move(jobs));
  }

  void Network::clear_wmes(const std::shared_ptr<Concurrency::Job_Queue> job_queue) {
    for (auto token : m_node_data.snapshot<NODE_DATA_SUBTRIE_TOKEN_OUTPUTS>())
      remove_wme(job_queue, dynamic_cast<const Token_Alpha &>(*token).get_wme());
  }

  void Network::source_rule(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node_Action> action, const bool user_command) {
    const auto[result, snapshot, inserted, replaced] = m_rules.insert(action);

    if (user_command && m_printed_output != Printed_Output::None) {
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
