#ifndef ZENI_RETE_NETWORK_HPP
#define ZENI_RETE_NETWORK_HPP

#include "Zeni/Concurrency/Container/Hash_Trie.hpp"
#include "Zeni/Concurrency/Recipient.hpp"
#include "Node.hpp"
#include "Node_Action.hpp"

#include <set>
#include <unordered_set>

namespace Zeni::Concurrency {

  class Job_Queue;
  class Worker_Threads;

}

namespace Zeni::Rete {

  class Node_Action;
  class Node_Filter;
  class WME;

  class Network : public Node {
    Network(const Network &) = delete;
    Network & operator=(const Network &) = delete;

    friend class Unlocked_Network_Data;
    friend class Locked_Network_Data_Const;
    friend class Locked_Network_Data;

  protected:
    ZENI_RETE_LINKAGE std::shared_ptr<const Network> shared_from_this() const;
    ZENI_RETE_LINKAGE std::shared_ptr<Network> shared_from_this();

  public:
    class Instantiation : public std::enable_shared_from_this<Instantiation> {
      Instantiation(const Instantiation &) = delete;
      Instantiation & operator=(const Instantiation &) = delete;

      ZENI_RETE_LINKAGE Instantiation(const std::shared_ptr<Network> network);

    public:
      ZENI_RETE_LINKAGE static std::shared_ptr<Instantiation> Create(const std::shared_ptr<Network> network);

      ZENI_RETE_LINKAGE ~Instantiation();

      ZENI_RETE_LINKAGE std::shared_ptr<const Network> get() const;
      ZENI_RETE_LINKAGE std::shared_ptr<Network> get();

      ZENI_RETE_LINKAGE const Network * operator*() const;
      ZENI_RETE_LINKAGE Network * operator*();
      ZENI_RETE_LINKAGE const Network * operator->() const;
      ZENI_RETE_LINKAGE Network * operator->();

    private:
      const std::shared_ptr<Network> m_network;
    };

    enum class ZENI_RETE_LINKAGE Printed_Output { Normal, None };

  private:
    Network(const Printed_Output printed_output);
    Network(const Printed_Output printed_output, const std::shared_ptr<Concurrency::Worker_Threads> &worker_threads);

    ZENI_RETE_LINKAGE void Destroy();

    ZENI_RETE_LINKAGE void send_disconnect_from_parents(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) override;

  public:
    ZENI_RETE_LINKAGE static std::shared_ptr<Instantiation> Create(const Printed_Output printed_output = Printed_Output::Normal);
    ZENI_RETE_LINKAGE static std::shared_ptr<Instantiation> Create(const Printed_Output printed_output, const std::shared_ptr<Concurrency::Worker_Threads> worker_threads);

    ZENI_RETE_LINKAGE ~Network();

    ZENI_RETE_LINKAGE std::shared_ptr<Concurrency::Worker_Threads> get_Worker_Threads() const;
    ZENI_RETE_LINKAGE std::shared_ptr<Node_Action> get_rule(const std::string &name) const;
    ZENI_RETE_LINKAGE std::set<std::string> get_rule_names() const;
    ZENI_RETE_LINKAGE int64_t get_rule_name_index() const;
    ZENI_RETE_LINKAGE void set_rule_name_index(const int64_t rule_name_index_);
    ZENI_RETE_LINKAGE Printed_Output get_Printed_Output() const;

    ZENI_RETE_LINKAGE std::pair<std::shared_ptr<Node>, std::shared_ptr<Node>> get_inputs() override;

    ZENI_RETE_LINKAGE void receive(const Message_Token_Insert &) override;
    ZENI_RETE_LINKAGE void receive(const Message_Token_Remove &) override;

    ZENI_RETE_LINKAGE void source_rule(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node_Action> action, const bool user_command);
    ZENI_RETE_LINKAGE void excise_all(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_command);
    ZENI_RETE_LINKAGE void excise_rule(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::string &name, const bool user_command);
    ZENI_RETE_LINKAGE std::string next_rule_name(const std::string_view prefix);
    ZENI_RETE_LINKAGE std::shared_ptr<Node_Action> unname_rule(const std::string &name, const bool user_command);

    ZENI_RETE_LINKAGE void insert_wme(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const WME> wme);
    ZENI_RETE_LINKAGE void remove_wme(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const WME> wme);
    ZENI_RETE_LINKAGE void clear_wmes(const std::shared_ptr<Concurrency::Job_Queue> job_queue);

    ZENI_RETE_LINKAGE bool operator==(const Node &rhs) const override;

  private:
    const std::shared_ptr<Concurrency::Worker_Threads> m_worker_threads;

    typedef Concurrency::Hash_Trie<std::shared_ptr<Node_Action>, Node_Action::Hash_By_Name, Node_Action::Compare_By_Name_Eq> Rule_Trie;
    Rule_Trie m_rules;

    Concurrency::Atomic_int64_t<false> m_rule_name_index = 0;

    // Options
    const Printed_Output m_printed_output;
  };

}

#endif
