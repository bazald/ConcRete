#ifndef ZENI_RETE_NETWORK_HPP
#define ZENI_RETE_NETWORK_HPP

#include "Zeni/Concurrency/Container/Hash_Trie.hpp"
#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Concurrency/Recipient.hpp"
#include "Node.hpp"
#include "Node_Action.hpp"

#include <set>
#include <unordered_set>

namespace Zeni::Concurrency {

  class Worker_Threads;

}

namespace Zeni::Rete {

  class Node_Action;
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
    struct Invalid_Worker_Threads_Setup : public std::runtime_error {
      Invalid_Worker_Threads_Setup() : std::runtime_error("Zeni::Rete::Network: Worker threads must be uninitialized before they can be (re)initialized!") {}
    };

    typedef Concurrency::Hash_Trie_S2<std::shared_ptr<const Symbol>, Concurrency::Super_Hash_Trie<Token_Trie, Node_Trie, Node_Trie>, hash_deref<Symbol>, compare_deref_eq> Symbol_Trie;
    typedef Concurrency::Super_Hash_Trie<Symbol_Trie, Node_Trie, Node_Trie> Filter_Layer_0_Trie;
    typedef Filter_Layer_0_Trie::Snapshot Filter_Layer_0_Snapshot;
    enum Filter_Layer_0 {
      FILTER_LAYER_0_SYMBOL = 0,
      FILTER_LAYER_0_VARIABLE_OUTPUTS = 1,
      FILTER_LAYER_0_VARIABLE_OUTPUTS_UNLINKED = 2
    };
    enum Filter_Layer_0_Symbol {
      FILTER_LAYER_0_SYMBOL_TOKENS = 0,
      FILTER_LAYER_0_SYMBOL_OUTPUTS = 1,
      FILTER_LAYER_0_SYMBOL_OUTPUTS_UNLINKED = 2
    };

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
    class Genatom : public Concurrency::Enable_Intrusive_Sharing {
      Genatom(const Genatom &) = delete;
      Genatom & operator=(const Genatom &) = delete;

    public:
      Genatom(const std::string &str = "0") : m_str(str) {}

      Concurrency::Intrusive_Shared_Ptr<Genatom>::Lock next() const;

      std::string_view str() const;

    private:
      const std::string m_str;
    };

    Network(const Printed_Output printed_output);
    Network(const Printed_Output printed_output, const std::shared_ptr<Concurrency::Worker_Threads> &worker_threads);

    ZENI_RETE_LINKAGE void Destroy();

    ZENI_RETE_LINKAGE void connect_to_parents_again(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) override;
    ZENI_RETE_LINKAGE void send_disconnect_from_parents(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue) override;

  public:
    ZENI_RETE_LINKAGE static std::shared_ptr<Instantiation> Create(const Printed_Output printed_output = Printed_Output::Normal);
    ZENI_RETE_LINKAGE static std::shared_ptr<Instantiation> Create(const Printed_Output printed_output, const std::shared_ptr<Concurrency::Worker_Threads> worker_threads);

    ZENI_RETE_LINKAGE ~Network();

    ZENI_RETE_LINKAGE std::shared_ptr<Concurrency::Worker_Threads> get_Worker_Threads() const;
    ZENI_RETE_LINKAGE std::shared_ptr<Node_Action> get_rule(const std::string &name) const;
    ZENI_RETE_LINKAGE std::set<std::string> get_rule_names() const;
    ZENI_RETE_LINKAGE void init_genatom(const std::string &str);
    ZENI_RETE_LINKAGE std::string get_genatom() const;
    ZENI_RETE_LINKAGE std::string genatom();
    ZENI_RETE_LINKAGE bool is_exit_requested() const;
    ZENI_RETE_LINKAGE void request_exit();
    ZENI_RETE_LINKAGE Printed_Output get_Printed_Output() const;

    ZENI_RETE_LINKAGE void finish_jobs();
    ZENI_RETE_LINKAGE void finish_jobs_and_destroy_worker_threads();
    ///< Worker threads must have been initialized to nullptr or destroyed first; Else throws Invalid_Worker_Threads_Setup()
    ZENI_RETE_LINKAGE void set_worker_threads(const std::shared_ptr<Concurrency::Worker_Threads> worker_threads);

    ZENI_RETE_LINKAGE std::pair<Node_Trie::Result, std::shared_ptr<Node>> connect_new_or_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) override;
    ZENI_RETE_LINKAGE Node_Trie::Result connect_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child, const bool unlinked) override;

    ZENI_RETE_LINKAGE Node_Trie::Result link_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) override;
    ZENI_RETE_LINKAGE Node_Trie::Result unlink_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) override;

    ZENI_RETE_LINKAGE bool has_tokens(const std::shared_ptr<const Node_Key> key) const override;

    ZENI_RETE_LINKAGE void receive(const Message_Token_Insert &) override;
    ZENI_RETE_LINKAGE void receive(const Message_Token_Remove &) override;
    ZENI_RETE_LINKAGE void receive(const Message_Connect_Filter_0 &message) override;
    ZENI_RETE_LINKAGE void receive(const Message_Disconnect_Output &message) override;

    ZENI_RETE_LINKAGE bool is_linked(const std::shared_ptr<Node> input, const std::shared_ptr<const Node_Key> key) override;

    ZENI_RETE_LINKAGE void source_rule(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node_Action> action, const bool user_action);
    ZENI_RETE_LINKAGE void excise_all(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const bool user_action);
    ZENI_RETE_LINKAGE void excise_rule(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::string &name, const bool user_action);
    ZENI_RETE_LINKAGE std::string next_rule_name(const std::string_view prefix);
    ZENI_RETE_LINKAGE std::shared_ptr<Node_Action> unname_rule(const std::string &name, const bool user_action);

    ZENI_RETE_LINKAGE void insert_wme(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const WME> wme);
    ZENI_RETE_LINKAGE void remove_wme(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const WME> wme);
    ZENI_RETE_LINKAGE void clear_wmes(const std::shared_ptr<Concurrency::Job_Queue> job_queue);

    ZENI_RETE_LINKAGE bool operator==(const Node &rhs) const override;

  private:
    ZENI_RETE_LINKAGE void insert_tokens(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child, const Filter_Layer_0_Snapshot snapshot);
    ZENI_RETE_LINKAGE void remove_tokens(const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child, const Filter_Layer_0_Snapshot snapshot);

    std::shared_ptr<Concurrency::Worker_Threads> m_worker_threads;

    Filter_Layer_0_Trie m_filter_layer_0_trie;

    typedef Concurrency::Hash_Trie<std::shared_ptr<Node_Action>, Node_Action::Hash_By_Name, Node_Action::Compare_By_Name_Eq> Rule_Trie;
    Rule_Trie m_rules;

    Concurrency::Intrusive_Shared_Ptr<Genatom> m_genatom = new Genatom();

    Concurrency::Atomic<bool> m_exit_requested = false;

    // Options
    const Printed_Output m_printed_output;
  };

}

#endif
