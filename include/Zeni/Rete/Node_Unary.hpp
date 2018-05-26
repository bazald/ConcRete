#ifndef ZENI_RETE_NODE_UNARY_H
#define ZENI_RETE_NODE_UNARY_H

#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete {

  class Node_Unary : public Node {
    Node_Unary(const Node_Unary &) = delete;
    Node_Unary & operator=(const Node_Unary &) = delete;

  public:
    class Locked_Node_Unary_Data_Const;
    class Locked_Node_Unary_Data;

    class Unlocked_Node_Unary_Data {
      Unlocked_Node_Unary_Data(const Unlocked_Node_Unary_Data &) = delete;
      Unlocked_Node_Unary_Data & operator=(const Unlocked_Node_Unary_Data &) = delete;

      friend Locked_Node_Unary_Data_Const;
      friend Locked_Node_Unary_Data;

    public:
      Unlocked_Node_Unary_Data(const std::shared_ptr<Pseudonode> input);

    private:
      std::shared_ptr<Pseudonode> m_input;
      Tokens m_input_tokens;
      Tokens m_input_antitokens;
    };

    class Locked_Node_Unary_Data_Const {
      Locked_Node_Unary_Data_Const(const Locked_Node_Unary_Data_Const &) = delete;
      Locked_Node_Unary_Data_Const & operator=(const Locked_Node_Unary_Data_Const &) = delete;

      friend Locked_Node_Unary_Data;

    public:
      ZENI_RETE_LINKAGE Locked_Node_Unary_Data_Const(const Node_Unary * node, const Locked_Node_Data_Const &node_data);

      ZENI_RETE_LINKAGE std::shared_ptr<const Pseudonode> get_input() const;
      ZENI_RETE_LINKAGE const Tokens & get_input_tokens() const;
      ZENI_RETE_LINKAGE const Tokens & get_input_antitokens() const;

    private:
      std::shared_ptr<const Unlocked_Node_Unary_Data> m_data;
    };

    class Locked_Node_Unary_Data : public Locked_Node_Unary_Data_Const {
      Locked_Node_Unary_Data(const Locked_Node_Unary_Data &) = delete;
      Locked_Node_Unary_Data & operator=(const Locked_Node_Unary_Data &) = delete;

    public:
      ZENI_RETE_LINKAGE Locked_Node_Unary_Data(Node_Unary * node, const Locked_Node_Data &node_data);

      ZENI_RETE_LINKAGE std::shared_ptr<Pseudonode> & modify_input();
      ZENI_RETE_LINKAGE Tokens & modify_input_tokens();
      ZENI_RETE_LINKAGE Tokens & modify_input_antitokens();

    private:
      std::shared_ptr<Unlocked_Node_Unary_Data> m_data;
    };

  protected:
    Node_Unary(const int64_t height, const int64_t size, const int64_t token_size, const std::shared_ptr<Pseudonode> input);

    ZENI_RETE_LINKAGE void send_disconnect_from_parents(const std::shared_ptr<Network> network, const Locked_Node_Data &locked_node_data) override;

    ZENI_RETE_LINKAGE bool receive(const Raven_Token_Insert &raven) override;
    ZENI_RETE_LINKAGE bool receive(const Raven_Token_Remove &raven) override;

  public:
    ZENI_RETE_LINKAGE std::shared_ptr<Pseudonode> get_input();

  private:
    std::shared_ptr<Unlocked_Node_Unary_Data> m_unlocked_node_unary_data;
  };

}

#endif
