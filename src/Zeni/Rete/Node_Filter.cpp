#include "Zeni/Rete/Node_Filter.hpp"

#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Raven_Token_Insert.hpp"
#include "Zeni/Rete/Raven_Token_Remove.hpp"

#include <cassert>

namespace Zeni {

  namespace Rete {

    Node_Filter::Node_Filter(const WME &wme_)
      : Node(1, 1, 1),
      m_wme(wme_)
    {
      for (int i = 0; i != 3; ++i)
        m_variable[i] = std::dynamic_pointer_cast<const Symbol_Variable>(m_wme.symbols[i]);
    }

    const WME & Node_Filter::get_wme() const {
      return m_wme;
    }

    std::shared_ptr<Node_Filter> Node_Filter::Create(const std::shared_ptr<Network> &network, const WME &wme) {
      class Friendly_Node_Filter : public Node_Filter {
      public:
        Friendly_Node_Filter(const WME &wme_) : Node_Filter(wme_) {}
      };

      auto filter = std::make_shared<Friendly_Node_Filter>(wme);

      if (network->get_Node_Sharing() == Network::Node_Sharing::Enabled) {
        const auto existing_filter = network->find_filter(filter);
        if (existing_filter)
          return existing_filter;
      }

      network->source_filter(filter);

      return filter;
    }

    void Node_Filter::receive(const Raven_Token_Insert &raven) {
      const auto &token = raven.get_Token();
      assert(token->size() == 1);
      const auto &wme = token->get_wme();

      for (int i = 0; i != 3; ++i)
        if (!m_variable[i] && *m_wme.symbols[i] != *wme->symbols[i])
          return;

      if (m_variable[0] && m_variable[1] && *m_variable[0] == *m_variable[1] && *wme->symbols[0] != *wme->symbols[1])
        return;
      if (m_variable[0] && m_variable[2] && *m_variable[0] == *m_variable[2] && *wme->symbols[0] != *wme->symbols[2])
        return;
      if (m_variable[1] && m_variable[2] && *m_variable[1] == *m_variable[2] && *wme->symbols[1] != *wme->symbols[2])
        return;

      Outputs outputs;
      std::shared_ptr<const Token> output_token;
      
      {
        Concurrency::Mutex::Lock lock(m_mutex);
        outputs = m_outputs;
        const auto inserted = m_output_tokens.insert(std::make_shared<Token>(wme));
        output_token = *inserted;
      }

      const auto sft = shared_from_this();
      for (auto &output : outputs)
        raven.get_Network()->get_Job_Queue()->give(std::make_shared<Raven_Token_Insert>(output, raven.get_Network(), sft, output_token));
    }

    void Node_Filter::receive(const Raven_Token_Remove &raven) {
      const auto &token = raven.get_Token();
      assert(token->size() == 1);
      const auto &wme = token->get_wme();

      Outputs outputs;
      std::shared_ptr<const Token> output_token;

      {
        Concurrency::Mutex::Lock lock(m_mutex);
        outputs = m_outputs;
        auto found = m_output_tokens.find(std::make_shared<Token>(wme));
        assert(found != m_output_tokens.end());
        output_token = *found;
        m_output_tokens.erase(found);
      }

      const auto sft = shared_from_this();
      for (auto &output : outputs)
        raven.get_Network()->get_Job_Queue()->give(std::make_shared<Raven_Token_Remove>(output, raven.get_Network(), sft, output_token));
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

  }

}
