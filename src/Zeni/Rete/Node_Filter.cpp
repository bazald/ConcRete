#include "Zeni/Rete/Node_Filter.hpp"

#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Token_Pass.hpp"

namespace Zeni {

  namespace Rete {

    Node_Filter::Node_Filter(const WME &wme_)
      : m_wme(wme_)
    {
      for (int i = 0; i != 3; ++i)
        m_variable[i] = std::dynamic_pointer_cast<const Symbol_Variable>(m_wme.symbols[i]);
    }

    const WME & Node_Filter::get_wme() const {
      return m_wme;
    }

    std::shared_ptr<Node_Filter> Node_Filter::Create(const WME &wme_) {
      class Friendly_Node_Filter : public Node_Filter {
      public:
        Friendly_Node_Filter(const WME &wme_) : Node_Filter(wme_) {}
      };

      return std::make_shared<Friendly_Node_Filter>(wme_);
    }

    void Node_Filter::Destroy(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output) {
      erase_output(output);
      if (!destruction_suppressed && outputs_all.empty())
        network->excise_filter(std::static_pointer_cast<Node_Filter>(shared_from_this()));
    }

    std::shared_ptr<const Node_Filter> Node_Filter::get_filter(const int64_t &
#ifndef NDEBUG
      index
#endif
    ) const {
      assert(index == 0);
      return std::static_pointer_cast<const Node_Filter>(shared_from_this());
    }

    const Node::Tokens & Node_Filter::get_output_tokens() const {
      return tokens;
    }

    bool Node_Filter::has_output_tokens() const {
      return !tokens.empty();
    }

    void Node_Filter::insert_wme(const std::shared_ptr<Network> &network, const std::shared_ptr<const WME> &wme) {
      for (int i = 0; i != 3; ++i)
        if (!m_variable[i] && *m_wme.symbols[i] != *wme->symbols[i])
          return;

      if (m_variable[0] && m_variable[1] && *m_variable[0] == *m_variable[1] && *wme->symbols[0] != *wme->symbols[1])
        return;
      if (m_variable[0] && m_variable[2] && *m_variable[0] == *m_variable[2] && *wme->symbols[0] != *wme->symbols[2])
        return;
      if (m_variable[1] && m_variable[2] && *m_variable[1] == *m_variable[2] && *wme->symbols[1] != *wme->symbols[2])
        return;

      const auto inserted = tokens.insert(std::make_shared<Token>(wme));
      if (inserted.second) {
        const auto sft = shared_from_this();
        for (auto &output : outputs_enabled)
          network->get_Job_Queue()->give(std::make_shared<Token_Pass>(output, network, sft, *inserted.first, Token_Pass::Type::Action));
      }
    }

    void Node_Filter::remove_wme(const std::shared_ptr<Network> &network, const std::shared_ptr<const WME> &wme) {
      auto found = tokens.find(std::make_shared<Token>(wme));
      if (found != tokens.end()) {
        const auto sft = shared_from_this();
        for (auto ot = outputs_enabled.begin(), oend = outputs_enabled.end(); ot != oend; )
          network->get_Job_Queue()->give(std::make_shared<Token_Pass>(*ot++, network, sft, *found, Token_Pass::Type::Retraction));
        tokens.erase(found);
      }
    }

    void Node_Filter::insert_token(const std::shared_ptr<Network> &, const std::shared_ptr<const Token> &, const std::shared_ptr<const Node> &) {
      abort();
    }

    void Node_Filter::remove_token(const std::shared_ptr<Network> &, const std::shared_ptr<const Token> &, const std::shared_ptr<const Node> &) {
      abort();
    }

    void Node_Filter::pass_tokens(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output) {
      const auto sft = shared_from_this();
      for (auto &token : tokens)
        network->get_Job_Queue()->give(std::make_shared<Token_Pass>(output, network, sft, token, Token_Pass::Type::Action));
    }

    void Node_Filter::unpass_tokens(const std::shared_ptr<Network> &network, const std::shared_ptr<Node> &output) {
      const auto sft = shared_from_this();
      for (auto &token : tokens)
        network->get_Job_Queue()->give(std::make_shared<Token_Pass>(output, network, sft, token, Token_Pass::Type::Retraction));
    }

    bool Node_Filter::operator==(const Node &rhs) const {
      if (auto filter = dynamic_cast<const Node_Filter *>(&rhs)) {
        for (int i = 0; i != 3; ++i) {
          if ((m_variable[i] != nullptr) ^ (filter->m_variable[i] != nullptr))
            return false;
          if (!m_variable[i] && *m_wme.symbols[i] != *filter->m_wme.symbols[i])
            return false;
        }

        if (m_variable[0] && m_variable[1] && ((*m_variable[0] == *m_variable[1]) ^ (*filter->m_variable[0] == *filter->m_variable[1])))
          return false;
        if (m_variable[0] && m_variable[2] && ((*m_variable[0] == *m_variable[2]) ^ (*filter->m_variable[0] == *filter->m_variable[2])))
          return false;
        if (m_variable[1] && m_variable[2] && ((*m_variable[1] == *m_variable[2]) ^ (*filter->m_variable[1] == *filter->m_variable[2])))
          return false;

        return true;
      }
      return false;
    }

    void Node_Filter::print_details(std::ostream &os) const {
      os << "  " << intptr_t(this) << " [label=\"F" << m_wme << "\"];" << std::endl;
    }

    void Node_Filter::print_rule(std::ostream &os, const std::shared_ptr<const Variable_Indices> &indices, const std::shared_ptr<const Node> &suppress) const {
      if (suppress && this == suppress->parent_left().get()) {
        os << '&' << dynamic_cast<const Node_Action *>(suppress.get())->get_name();
        return;
      }

      m_wme.print(os, indices);
    }

    void Node_Filter::output_name(std::ostream &os, const int64_t &) const {
      os << 'f' << m_wme;
    }

    bool Node_Filter::is_active() const {
      return !tokens.empty();
    }

    std::vector<WME> Node_Filter::get_filter_wmes() const {
      return std::vector<WME>(1, m_wme);
    }

    void bind_to_filter(const std::shared_ptr<Network> &/*network*/, const std::shared_ptr<Node_Filter> &filter) {
      assert(filter);
      filter->height = 1;
      filter->token_owner = filter;
      filter->size = 1;
      filter->token_size = 1;
    }

  }

}
