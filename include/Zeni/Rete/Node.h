#ifndef ZENI_NODE_H
#define ZENI_NODE_H

#include "Custom_Data.h"
#include "Token.h"

//#include <algorithm>
//#include <cassert>
//#include <functional>
//#include <list>
//#include <unordered_set>
#include <deque>

namespace Zeni {

  namespace Rete {

    class Agenda;
    class Network;
    class Node;
    class Node_Action;
    class Node_Existential;
    class Node_Filter;
    class Node_Join;
    class Node_Join_Existential;
    class Node_Join_Negation;
    class Node_Negation;
    class Node_Predicate;

    class ZENI_RETE_LINKAGE Node : public std::enable_shared_from_this<Node>
    {
      Node(const Node &) = delete;
      Node & operator=(const Node &) = delete;

      friend ZENI_RETE_LINKAGE void bind_to_action(Network &network, const std::shared_ptr<Node_Action> &action, const std::shared_ptr<Node> &out);
      friend ZENI_RETE_LINKAGE void bind_to_existential(Network &network, const std::shared_ptr<Node_Existential> &existential, const std::shared_ptr<Node> &out);
      friend ZENI_RETE_LINKAGE void bind_to_join_existential(Network &network, const std::shared_ptr<Node_Join_Existential> &join, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1);
      friend ZENI_RETE_LINKAGE void bind_to_filter(Network &network, const std::shared_ptr<Node_Filter> &filter);
      friend ZENI_RETE_LINKAGE void bind_to_join(Network &network, const std::shared_ptr<Node_Join> &join, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1);
      friend ZENI_RETE_LINKAGE void bind_to_negation(Network &network, const std::shared_ptr<Node_Negation> &negation, const std::shared_ptr<Node> &out);
      friend ZENI_RETE_LINKAGE void bind_to_join_negation(Network &network, const std::shared_ptr<Node_Join_Negation> &join, const std::shared_ptr<Node> &out0, const std::shared_ptr<Node> &out1);
      friend ZENI_RETE_LINKAGE void bind_to_predicate(Network &network, const std::shared_ptr<Node_Predicate> &predicate, const std::shared_ptr<Node> &out);

    public:
      typedef std::list<std::shared_ptr<Node_Filter>> Filters;
      typedef std::list<std::shared_ptr<Node>> Output_Ptrs;

      typedef std::unordered_set<std::shared_ptr<Node>> Outputs;
      typedef std::unordered_set<std::shared_ptr<const Token>, Zeni::hash_deref<Token>, Zeni::compare_deref_eq> Tokens;

      Node() {}

      virtual void destroy(Network &network, const std::shared_ptr<Node> &output) = 0;

      void suppress_destruction(const bool &suppress) {
        destruction_suppressed = suppress;
      }

      //std::shared_ptr<const Node> shared() const {
      //  return shared_from_this();
      //}
      //std::shared_ptr<Node> shared() {
      //  return shared_from_this();
      //}

      const Output_Ptrs & get_outputs_all() const {
        return outputs_all;
      }

      virtual std::shared_ptr<const Node> parent_left() const = 0;
      virtual std::shared_ptr<const Node> parent_right() const = 0;
      virtual std::shared_ptr<Node> parent_left() = 0;
      virtual std::shared_ptr<Node> parent_right() = 0;

      int64_t get_height() const { return height; }
      std::shared_ptr<const Node> get_token_owner() const { return token_owner.lock(); }
      int64_t get_size() const { return size; }
      int64_t get_token_size() const { return token_size; }

      virtual std::shared_ptr<const Node_Filter> get_filter(const int64_t &index) const = 0;

      virtual const Tokens & get_output_tokens() const = 0;
      virtual bool has_output_tokens() const = 0;

      virtual void insert_token(Network &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &from) = 0;
      virtual bool remove_token(Network &network, const std::shared_ptr<const Token> &token, const std::shared_ptr<const Node> &from) = 0; ///< Returns true if removed the last

      virtual void disconnect(Network &/*network*/, const std::shared_ptr<const Node> &/*from*/) {}

      virtual void pass_tokens(Network &network, const std::shared_ptr<Node> &output) = 0;
      virtual void unpass_tokens(Network &network, const std::shared_ptr<Node> &output) = 0;

      virtual bool operator==(const Node &rhs) const = 0;

      virtual bool disabled_input(const std::shared_ptr<Node> &);

      /// Disable an enabled output
      void disable_output(Network &network, const std::shared_ptr<Node> &output);
      /// Enable a disabled output
      void enable_output(Network &network, const std::shared_ptr<Node> &output);

      /// Add a new enabled output
      void insert_output_enabled(const std::shared_ptr<Node> &output);
      /// Add a new disabled output
      void insert_output_disabled(const std::shared_ptr<Node> &output);

      void erase_output(const std::shared_ptr<Node> &output);
      void erase_output_enabled(const std::shared_ptr<Node> &output);
      void erase_output_disabled(const std::shared_ptr<Node> &output);

      virtual void print_details(std::ostream &os) const = 0; ///< Formatted for dot: http://www.graphviz.org/content/dot-language

      virtual void print_rule(std::ostream &os, const std::shared_ptr<const Variable_Indices> &indices, const std::shared_ptr<const Node> &suppress) const = 0;

      virtual void output_name(std::ostream &os, const int64_t &depth) const = 0;

      virtual bool is_active() const = 0; ///< Has the node matched and forwarded at least one token?

      virtual std::vector<WME> get_filter_wmes() const = 0;

      virtual const Variable_Bindings * get_bindings() const { return nullptr; }

      template <typename VISITOR>
      VISITOR visit_preorder(VISITOR visitor, const bool &strict) {
        return visit_preorder(visitor, strict, (intptr_t(this) & ~intptr_t(3)) | ((visitor_value & 3) != 1 ? 1 : 2));
      }

      template <typename VISITOR>
      VISITOR visit_preorder(VISITOR visitor, const bool &strict, const intptr_t &visitor_value_);

      std::shared_ptr<Custom_Data> custom_data;

    protected:
      bool destruction_suppressed = false;
      Output_Ptrs outputs_all;
      Outputs outputs_enabled;
      Outputs outputs_disabled;

      int64_t height = 0;
      std::weak_ptr<const Node> token_owner;
      int64_t size = -1;
      int64_t token_size = -1;

    private:
      intptr_t visitor_value = 0;
    };

  }

}

#endif
#include "Node_Filter.h"
#if !defined(Node_H_PART2) && defined(RETE_FILTER_H_DONE)
#define Node_H_PART2

namespace Zeni {

  namespace Rete {

    template <typename VISITOR>
    VISITOR Node::visit_preorder(VISITOR visitor, const bool &strict, const intptr_t &visitor_value_) {
      std::deque<std::shared_ptr<Node>> nodes;
      nodes.push_back(shared_from_this());

      while(!nodes.empty()) {
        const std::shared_ptr<Node> node = nodes.front();
        nodes.pop_front();

        if(node->visitor_value == visitor_value_)
          continue;

        if(!strict && !std::dynamic_pointer_cast<Node_Filter>(node)) {
          if(node->parent_left()->visitor_value != visitor_value_) {
            nodes.push_front(node->parent_left());
            continue;
          }
          else if(node->parent_right()->visitor_value != visitor_value_) {
            nodes.push_front(node->parent_right());
            continue;
          }
        }

        node->visitor_value = visitor_value_;
        visitor(node);
        for(auto &o : node->outputs_all)
          nodes.push_back(o);
      }

      return visitor;
    }

  }

}

#endif
