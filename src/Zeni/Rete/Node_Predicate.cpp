#include "Zeni/Rete/Node_Predicate.hpp"

#include "Zeni/Concurrency/Job_Queue.hpp"
#include "Zeni/Rete/Internal/Message_Connect_Predicate.hpp"
#include "Zeni/Rete/Internal/Message_Disconnect_Output.hpp"
#include "Zeni/Rete/Internal/Message_Token_Insert.hpp"
#include "Zeni/Rete/Internal/Message_Token_Remove.hpp"
#include "Zeni/Rete/Internal/Token_Alpha.hpp"
#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Node_Key.hpp"

#include <cassert>

namespace Zeni::Rete {

  std::shared_ptr<const Node_Predicate::Predicate_E> Node_Predicate::Predicate_E::Create() {
    class Friendly_Predicate_E : public Predicate_E {
    public:
      Friendly_Predicate_E() {}
    };

    static const auto predicate_exec_e = std::make_shared<Friendly_Predicate_E>();
    return predicate_exec_e;
  }

  bool Node_Predicate::Predicate_E::operator()(const std::shared_ptr<const Symbol> lhs, const std::shared_ptr<const Symbol> rhs) const {
    return *lhs == *rhs;
  }

  size_t Node_Predicate::Predicate_E::hash() const {
    return 1;
  }

  std::shared_ptr<const Node_Predicate::Predicate_NE> Node_Predicate::Predicate_NE::Create() {
    class Friendly_Predicate_NE : public Predicate_NE {
    public:
      Friendly_Predicate_NE() {}
    };

    static const auto predicate_exec_ne = std::make_shared<Friendly_Predicate_NE>();
    return predicate_exec_ne;
  }

  bool Node_Predicate::Predicate_NE::operator()(const std::shared_ptr<const Symbol> lhs, const std::shared_ptr<const Symbol> rhs) const {
    return *lhs != *rhs;
  }

  size_t Node_Predicate::Predicate_NE::hash() const {
    return 2;
  }

  std::shared_ptr<const Node_Predicate::Predicate_GT> Node_Predicate::Predicate_GT::Create() {
    class Friendly_Predicate_GT : public Predicate_GT {
    public:
      Friendly_Predicate_GT() {}
    };

    static const auto predicate_exec_gt = std::make_shared<Friendly_Predicate_GT>();
    return predicate_exec_gt;
  }

  bool Node_Predicate::Predicate_GT::operator()(const std::shared_ptr<const Symbol> lhs, const std::shared_ptr<const Symbol> rhs) const {
    return *lhs > *rhs;
  }

  size_t Node_Predicate::Predicate_GT::hash() const {
    return 3;
  }

  std::shared_ptr<const Node_Predicate::Predicate_GTE> Node_Predicate::Predicate_GTE::Create() {
    class Friendly_Predicate_GTE : public Predicate_GTE {
    public:
      Friendly_Predicate_GTE() {}
    };

    static const auto predicate_exec_gte = std::make_shared<Friendly_Predicate_GTE>();
    return predicate_exec_gte;
  }

  bool Node_Predicate::Predicate_GTE::operator()(const std::shared_ptr<const Symbol> lhs, const std::shared_ptr<const Symbol> rhs) const {
    return *lhs >= *rhs;
  }

  size_t Node_Predicate::Predicate_GTE::hash() const {
    return 4;
  }

  std::shared_ptr<const Node_Predicate::Predicate_LT> Node_Predicate::Predicate_LT::Create() {
    class Friendly_Predicate_LT : public Predicate_LT {
    public:
      Friendly_Predicate_LT() {}
    };

    static const auto predicate_exec_lt = std::make_shared<Friendly_Predicate_LT>();
    return predicate_exec_lt;
  }

  bool Node_Predicate::Predicate_LT::operator()(const std::shared_ptr<const Symbol> lhs, const std::shared_ptr<const Symbol> rhs) const {
    return *lhs < *rhs;
  }

  size_t Node_Predicate::Predicate_LT::hash() const {
    return 5;
  }

  std::shared_ptr<const Node_Predicate::Predicate_LTE> Node_Predicate::Predicate_LTE::Create() {
    class Friendly_Predicate_LTE : public Predicate_LTE {
    public:
      Friendly_Predicate_LTE() {}
    };

    static const auto predicate_exec_lte = std::make_shared<Friendly_Predicate_LTE>();
    return predicate_exec_lte;
  }

  bool Node_Predicate::Predicate_LTE::operator()(const std::shared_ptr<const Symbol> lhs, const std::shared_ptr<const Symbol> rhs) const {
    return *lhs <= *rhs;
  }

  size_t Node_Predicate::Predicate_LTE::hash() const {
    return 6;
  }

  std::shared_ptr<const Node_Predicate::Predicate_STA> Node_Predicate::Predicate_STA::Create() {
    class Friendly_Predicate_STA : public Predicate_STA {
    public:
      Friendly_Predicate_STA() {}
    };

    static const auto predicate_exec_sta = std::make_shared<Friendly_Predicate_STA>();
    return predicate_exec_sta;
  }

  bool Node_Predicate::Predicate_STA::operator()(const std::shared_ptr<const Symbol> lhs, const std::shared_ptr<const Symbol> rhs) const {
    return lhs->is_same_type_as(*rhs);
  }

  size_t Node_Predicate::Predicate_STA::hash() const {
    return 7;
  }

  bool Node_Predicate::Get_Symbol::operator==(const Get_Symbol_Constant &) const {
    return false;
  }

  bool Node_Predicate::Get_Symbol::operator==(const Get_Symbol_Variable &) const {
    return false;
  }

  size_t Node_Predicate::Get_Symbol_Constant::hash() const {
    return symbol->hash();
  }

  bool Node_Predicate::Get_Symbol_Constant::operator==(const Get_Symbol &rhs) const {
    return rhs == *this;
  }

  bool Node_Predicate::Get_Symbol_Constant::operator==(const Get_Symbol_Constant &rhs) const {
    return *symbol == *rhs.symbol;
  }

  size_t Node_Predicate::Get_Symbol_Variable::hash() const {
    return std::hash<Token_Index>()(index);
  }

  bool Node_Predicate::Get_Symbol_Variable::operator==(const Get_Symbol &rhs) const {
    return rhs == *this;
  }

  bool Node_Predicate::Get_Symbol_Variable::operator==(const Get_Symbol_Variable &rhs) const {
    return index == rhs.index;
  }

  Node_Predicate::Get_Symbol_Constant::Get_Symbol_Constant(const std::shared_ptr<const Symbol> symbol_)
    : symbol(symbol_)
  {
    assert(symbol_);
  }

  Node_Predicate::Get_Symbol_Variable::Get_Symbol_Variable(const Token_Index index_)
    : index(index_)
  {
    assert(index_ != Token_Index());
  }

  std::shared_ptr<const Symbol> Node_Predicate::Get_Symbol_Constant::get_symbol(const std::shared_ptr<const Token>) const {
    return symbol;
  }

  std::shared_ptr<const Symbol> Node_Predicate::Get_Symbol_Variable::get_symbol(const std::shared_ptr<const Token> token) const {
    return (*token)[index];
  }

  Node_Predicate::Node_Predicate(const std::shared_ptr<Network> network, const std::shared_ptr<const Node_Key> node_key, const std::shared_ptr<Node> input, const std::shared_ptr<const Predicate> predicate, const Token_Index &lhs, const std::shared_ptr<const Get_Symbol> rhs)
    : Node_Unary(1, 1, 1, hash_combine(hash_combine(hash_combine(hash_combine(std::hash<int>()(6), node_key->hash()), predicate->hash()), std::hash<Token_Index>()(lhs)), rhs->hash()), node_key, input),
    m_predicate(predicate),
    m_lhs_index(lhs),
    m_rhs(rhs)
  {
  }

  std::shared_ptr<Node_Predicate> Node_Predicate::Create(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> node_key, const std::shared_ptr<Node> input, const std::shared_ptr<const Predicate> predicate, const Token_Index &lhs, const std::shared_ptr<const Get_Symbol> rhs) {
    class Friendly_Node_Predicate : public Node_Predicate {
    public:
      Friendly_Node_Predicate(const std::shared_ptr<Network> &network, const std::shared_ptr<const Node_Key> node_key, const std::shared_ptr<Node> input, const std::shared_ptr<const Predicate> predicate, const Token_Index &lhs, const std::shared_ptr<const Get_Symbol> rhs) : Node_Predicate(network, node_key, input, predicate, lhs, rhs) {}
    };

    const auto created = std::shared_ptr<Friendly_Node_Predicate>(new Friendly_Node_Predicate(network, node_key, input, predicate, lhs, rhs));

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

    return std::static_pointer_cast<Node_Predicate>(connected);
  }

  std::pair<Node::Node_Trie::Result, std::shared_ptr<Node>> Node_Predicate::connect_new_or_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) {
    const auto sft = shared_from_this();

    assert(dynamic_cast<const Node_Key_Null *>(key.get()));

    const auto[result, snapshot, value] = m_predicate_layer_trie.insert_ip_xp<PREDICATE_LAYER_OUTPUTS_UNLINKED, PREDICATE_LAYER_OUTPUTS>(child);

    if (result == Node_Trie::Result::First_Insertion)
      job_queue->give_one(std::make_shared<Message_Connect_Predicate>(sft, network, std::move(snapshot), value));

    return std::make_pair(result, value);
  }

  Node::Node_Trie::Result Node_Predicate::connect_existing_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child, const bool unlinked) {
    const auto sft = shared_from_this();

    assert(dynamic_cast<const Node_Key_Null *>(key.get()));

    const auto[result, snapshot, value] = unlinked
      ? m_predicate_layer_trie.insert_ip_xp<PREDICATE_LAYER_OUTPUTS, PREDICATE_LAYER_OUTPUTS_UNLINKED>(child)
      : m_predicate_layer_trie.insert_ip_xp<PREDICATE_LAYER_OUTPUTS_UNLINKED, PREDICATE_LAYER_OUTPUTS>(child);

    assert(value == child);

    if (!unlinked && result == Node_Trie::Result::First_Insertion)
      job_queue->give_one(std::make_shared<Message_Connect_Predicate>(sft, network, std::move(snapshot), value));

    return result;
  }

  Node::Node_Trie::Result Node_Predicate::link_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) {
    assert(dynamic_cast<const Node_Key_Null *>(key.get()));

    const auto[result, snapshot, value] = m_predicate_layer_trie.move<PREDICATE_LAYER_OUTPUTS_UNLINKED, PREDICATE_LAYER_OUTPUTS>(child);

    if (result != Node_Trie::Result::Successful_Move)
      return result;

    insert_tokens(network, job_queue, child, snapshot);

    return result;
  }

  Node::Node_Trie::Result Node_Predicate::unlink_output(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<const Node_Key> key, const std::shared_ptr<Node> child) {
    assert(dynamic_cast<const Node_Key_Null *>(key.get()));

    const auto[result, snapshot, value] = m_predicate_layer_trie.move<PREDICATE_LAYER_OUTPUTS, PREDICATE_LAYER_OUTPUTS_UNLINKED>(child);

    if (result != Node_Trie::Result::Successful_Move)
      return result;

    remove_tokens(network, job_queue, child, snapshot);

    return result;
  }

  bool Node_Predicate::has_tokens(const std::shared_ptr<const Node_Key> key) const {
    assert(dynamic_cast<const Node_Key_Null *>(key.get()));

    const auto tokens = m_predicate_layer_trie.snapshot<PREDICATE_LAYER_TOKENS>();
    return !tokens.size_zero();
  }

  void Node_Predicate::receive(const Message_Token_Insert &message) {
    if (!((*m_predicate)((*message.token)[m_lhs_index], m_rhs->get_symbol(message.token))))
      return;

    const auto[result, snapshot, value] = m_predicate_layer_trie.insert<PREDICATE_LAYER_TOKENS>(message.token);
    if (result != Token_Trie::Result::First_Insertion)
      return;

    const auto sft = shared_from_this();
    const auto &job_queue = message.get_Job_Queue();

    for (auto &output : snapshot.snapshot<PREDICATE_LAYER_OUTPUTS>())
      job_queue->give_one(std::make_shared<Message_Token_Insert>(output, message.network, sft, Node_Key_Null::Create(), message.token));
  }

  void Node_Predicate::receive(const Message_Token_Remove &message) {
    if (!((*m_predicate)((*message.token)[m_lhs_index], m_rhs->get_symbol(message.token))))
      return;

    const auto[result, snapshot, value] = m_predicate_layer_trie.erase<PREDICATE_LAYER_TOKENS>(message.token);
    if (result != Token_Trie::Result::Last_Removal)
      return;

    const auto sft = shared_from_this();
    const auto &job_queue = message.get_Job_Queue();

    for (auto &output : snapshot.snapshot<PREDICATE_LAYER_OUTPUTS>())
      job_queue->give_one(std::make_shared<Message_Token_Remove>(output, message.network, sft, Node_Key_Null::Create(), message.token));
  }

  void Node_Predicate::receive(const Message_Connect_Predicate &message) {
    insert_tokens(message.network, message.get_Job_Queue(), message.child, message.snapshot);
  }

  void Node_Predicate::receive(const Message_Disconnect_Output &message) {
    assert(std::dynamic_pointer_cast<const Node_Key_Null>(message.key));

    const auto[result, snapshot, value] = m_predicate_layer_trie.erase_ip_xp<PREDICATE_LAYER_OUTPUTS, PREDICATE_LAYER_OUTPUTS_UNLINKED>(message.child);

    assert(result != Node_Trie::Result::Failed_Removal);

    if (result != Node_Trie::Result::Last_Removal_IP && result != Node_Trie::Result::Last_Removal)
      return;

    send_disconnect_from_parents(message.network, message.get_Job_Queue());

    if (result != Node_Trie::Result::Last_Removal_IP)
      return;

    remove_tokens(message.network, message.get_Job_Queue(), message.child, snapshot);
  }

  void Node_Predicate::insert_tokens(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> child, const Predicate_Layer_Snapshot snapshot) {
    const auto sft = shared_from_this();

    for (const auto &token : snapshot.snapshot<PREDICATE_LAYER_TOKENS>())
      job_queue->give_one(std::make_shared<Message_Token_Insert>(child, network, sft, Node_Key_Null::Create(), token));
  }

  void Node_Predicate::remove_tokens(const std::shared_ptr<Network> network, const std::shared_ptr<Concurrency::Job_Queue> job_queue, const std::shared_ptr<Node> child, const Predicate_Layer_Snapshot snapshot) {
    const auto sft = shared_from_this();

    for (const auto &token : snapshot.snapshot<PREDICATE_LAYER_TOKENS>())
      job_queue->give_one(std::make_shared<Message_Token_Remove>(child, network, sft, Node_Key_Null::Create(), token));
  }

  bool Node_Predicate::operator==(const Node &rhs) const {
    //return this == &rhs;

    if (auto predicate = dynamic_cast<const Node_Predicate *>(&rhs)) {
      return *get_key() == *predicate->get_key() && get_input() == predicate->get_input()
        && m_predicate == predicate->m_predicate && m_lhs_index == predicate->m_lhs_index && *m_rhs == *predicate->m_rhs;
    }

    return false;
  }

}
