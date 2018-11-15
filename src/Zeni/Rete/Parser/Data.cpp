#include "Zeni/Rete/Parser/Data.hpp"

#include "Zeni/Rete/Node.hpp"

namespace Zeni::Rete::PEG {

  Symbol_Constant_Generator::Symbol_Constant_Generator(const std::shared_ptr<const Symbol> symbol)
    : m_symbol(symbol)
  {
    assert(!dynamic_cast<const Symbol_Variable *>(m_symbol.get()));
  }

  std::shared_ptr<const Symbol_Generator> Symbol_Constant_Generator::clone(const Symbol_Substitutions &) const {
    return shared_from_this();
  }

  std::shared_ptr<const Symbol> Symbol_Constant_Generator::generate(const Symbol_Substitutions &) const {
    return m_symbol;
  }

  Symbol_Variable_Generator::Symbol_Variable_Generator(const std::shared_ptr<const Symbol_Variable> symbol)
    : m_symbol(symbol)
  {
  }

  std::shared_ptr<const Symbol_Generator> Symbol_Variable_Generator::clone(const Symbol_Substitutions &substitutions) const {
    const auto found = substitutions.find(m_symbol);
    if (found == substitutions.end())
      return shared_from_this();
    else
      return std::make_shared<Symbol_Constant_Generator>(found->second);
  }

  std::shared_ptr<const Symbol> Symbol_Variable_Generator::generate(const Symbol_Substitutions &substitutions) const {
    const auto found = substitutions.find(m_symbol);
    return found == substitutions.end() ? m_symbol : found->second;
  }

  Data::Production::Production(const std::shared_ptr<Network> network_, const std::shared_ptr<Concurrency::Job_Queue> job_queue_, const bool user_command_)
    : network(network_), job_queue(job_queue_), user_command(user_command_)
  {
    lhs.push(decltype(lhs)::value_type());
  }

  Data::Production::~Production() {
    while (!lhs.empty()) {
      while (!lhs.top().empty()) {
        lhs.top().top().first.first->send_disconnect_from_parents(network, job_queue);
        lhs.top().pop();
      }
      lhs.pop();
    }
  }

  Data::Data(const std::shared_ptr<Network> network_, const std::shared_ptr<Concurrency::Job_Queue> job_queue_, const bool user_command_)
    : network(network_), job_queue(job_queue_)
  {
    productions.push(std::make_shared<Production>(network_, job_queue_, user_command_));
  }

}
