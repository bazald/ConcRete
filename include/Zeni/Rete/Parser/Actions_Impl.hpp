#ifndef ZENI_RETE_PARSER_ACTIONS_IMPL_HPP
#define ZENI_RETE_PARSER_ACTIONS_IMPL_HPP

#include "Actions.hpp"

#include "Zeni/Concurrency/Worker_Threads.hpp"
#include "Zeni/Rete/Parser/Data.hpp"
#include "Zeni/Rete/Node_Action.hpp"
#include "Zeni/Rete/Node_Filter_1.hpp"
#include "Zeni/Rete/Node_Filter_2.hpp"
#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Node_Join.hpp"
#include "Zeni/Rete/Node_Join_Existential.hpp"
#include "Zeni/Rete/Node_Join_Negation.hpp"
#include "Zeni/Rete/Parser.hpp"

namespace Zeni::Rete::PEG {

  /// Symbols

  template <typename Input>
  void Action<Constant_Float>::apply(const Input &input, Data &data) {
    const double d = std::stod(input.string());
    //std::cerr << "Constant_Float: " << d << std::endl;
    const auto symbol = std::make_shared<Symbol_Constant_Float>(d);
    data.symbols.emplace_back(std::make_shared<Symbol_Constant_Generator>(symbol));
  }

  template<typename Input>
  void Action<Constant_Int>::apply(const Input &input, Data &data) {
    const int64_t i = std::stoll(input.string());
    //std::cerr << "Constant_Int: " << i << std::endl;
    const auto symbol = std::make_shared<Symbol_Constant_Int>(i);
    data.symbols.emplace_back(std::make_shared<Symbol_Constant_Generator>(symbol));
  }

  template<typename Input>
  void Action<Constant_String>::apply(const Input &input, Data &data) {
    //std::cerr << "Constant_String: " << input.string() << std::endl;
    const auto symbol = std::make_shared<Symbol_Constant_String>(input.string());
    data.symbols.emplace_back(std::make_shared<Symbol_Constant_Generator>(symbol));
  }

  template<typename Input>
  void Action<Quoted_Constant_String>::apply(const Input &input, Data &data) {
    //std::cerr << "Quoted_Constant_String: " << input.string() << std::endl;
    assert(input.string().size() > 2);
    const auto substr = input.string().substr(1, input.string().size() - 2);
    const auto symbol = std::make_shared<Symbol_Constant_String>(substr);
    data.symbols.emplace_back(std::make_shared<Symbol_Constant_Generator>(symbol));
  }

  template<typename Input>
  void Action<CRLF>::apply(const Input &input, Data &data) {
    //std::cerr << "CRLF: " << input.string() << std::endl;
    const auto symbol = std::make_shared<Symbol_Constant_String>("\r\n");
    data.symbols.emplace_back(std::make_shared<Symbol_Constant_Generator>(symbol));
  }

  template<typename Input>
  void Action<Variable>::apply(const Input &input, Data &data) {
    //std::cerr << "Constant_Variable: " << input.string() << std::endl;
    assert(input.string().size() > 2);
    const auto substr = input.string().substr(1, input.string().size() - 2);
    if (data.productions.top()->phase == Data::Production::Phase::PHASE_LHS)
      data.productions.top()->lhs_variables.insert(substr);
    else if (data.productions.top()->lhs_variables.find(substr) == data.productions.top()->lhs_variables.end())
      throw parse_error("Parser error: variable used in the RHS not found in the LHS", input);
    const auto symbol = std::make_shared<Symbol_Variable>(substr.c_str());
    data.symbols.emplace_back(std::make_shared<Symbol_Variable_Generator>(symbol));
  }

  template<typename Input>
  void Action<Unnamed_Variable>::apply(const Input &input, Data &data) {
    //std::cerr << "Unnamed_Variable: " << input.string() << std::endl;
    const auto symbol = std::make_shared<Symbol_Variable>("");
    data.symbols.emplace_back(std::make_shared<Symbol_Variable_Generator>(symbol));
  }

  /// Conditions and WMEs

  template<typename Input>
  void Action<Condition_Begin>::apply(const Input &input, Data &data) {
    //std::cerr << "Condition_Begin: " << input.string() << std::endl;

    data.productions.top()->lhs.emplace(typename decltype(data.productions.top()->lhs)::value_type());
  }

  template<typename Input>
  void Action<Condition_Attr_Value>::apply(const Input &input, Data &data) {
    //std::cerr << "Condition_Attr_Value: " << input.string() << std::endl;

    assert(data.symbols.size() == 3);
    const auto third = data.symbols.back();
    data.symbols.pop_back();
    const auto second = data.symbols.back();
    data.symbols.pop_back();
    const auto first = data.symbols.back();
    // Deliberately leave the first symbol on the stack

    data.productions.top()->lhs.top().push(std::make_shared<Node_Filter_Generator>(first, second, third));

    if (data.productions.top()->lhs.top().size() == 2) {
      const auto right = data.productions.top()->lhs.top().top();
      data.productions.top()->lhs.top().pop();
      const auto left = data.productions.top()->lhs.top().top();
      data.productions.top()->lhs.top().pop();

      data.productions.top()->lhs.top().push(std::make_shared<Node_Join_Generator>(left, right));
    }
  }

  template<typename Input>
  void Action<Condition_End>::apply(const Input &input, Data &data) {
    //std::cerr << "Condition_End: " << input.string() << std::endl;

    assert(data.symbols.size() == 1);
    data.symbols.pop_back();

    assert(data.productions.top()->lhs.top().size() == 1);
    const auto node = data.productions.top()->lhs.top().top();
    data.productions.top()->lhs.pop();
    data.productions.top()->lhs.top().emplace(node);
  }

  /// Left-Hand Side / LHS

  template<typename Input>
  void Action<Inner_Scope_Begin>::apply(const Input &input, Data &data) {
    //std::cerr << "Inner_Scope_Begin: " << input.string() << std::endl;

    data.productions.top()->lhs.emplace(typename decltype(data.productions.top()->lhs)::value_type());
  }

  template<typename Input>
  void Action<Inner_Scope_End>::apply(const Input &input, Data &data) {
    //std::cerr << "Inner_Scope_End: " << input.string() << std::endl;

    assert(data.productions.top()->lhs.top().size() == 1);
    auto top = data.productions.top()->lhs.top().top();
    data.productions.top()->lhs.pop();
    data.productions.top()->lhs.top().emplace(std::move(top));
  }

  template<typename Input>
  void Action<Join>::apply(const Input &input, Data &data) {
    //std::cerr << "Join: " << input.string() << std::endl;

    assert(data.productions.top()->lhs.top().size() == 2);
    const auto right = data.productions.top()->lhs.top().top();
    data.productions.top()->lhs.top().pop();
    const auto left = data.productions.top()->lhs.top().top();
    data.productions.top()->lhs.top().pop();

    data.productions.top()->lhs.top().push(std::make_shared<Node_Join_Generator>(left, right));
  }

  template<typename Input>
  void Action<Join_Existential>::apply(const Input &input, Data &data) {
    //std::cerr << "Join_Existential: " << input.string() << std::endl;

    assert(data.productions.top()->lhs.top().size() == 2);
    const auto right = data.productions.top()->lhs.top().top();
    data.productions.top()->lhs.top().pop();
    const auto left = data.productions.top()->lhs.top().top();
    data.productions.top()->lhs.top().pop();

    data.productions.top()->lhs.top().push(std::make_shared<Node_Join_Existential_Generator>(left, right));
  }

  template<typename Input>
  void Action<Join_Negation>::apply(const Input &input, Data &data) {
    //std::cerr << "Join_Negation: " << input.string() << std::endl;

    assert(data.productions.top()->lhs.top().size() == 2);
    const auto right = data.productions.top()->lhs.top().top();
    data.productions.top()->lhs.top().pop();
    const auto left = data.productions.top()->lhs.top().top();
    data.productions.top()->lhs.top().pop();

    data.productions.top()->lhs.top().push(std::make_shared<Node_Join_Negation_Generator>(left, right));
  }

  /// Right-Hand Side / RHS

  template<typename Input>
  void Action<Excise>::apply(const Input &input, Data &data) {
    //std::cerr << "Excise: " << input.string() << std::endl;

    const auto excise = std::make_shared<Action_Excise_Generator>(data.symbols);
    data.symbols.clear();

    data.productions.top()->actions_or_retractions->push_back(excise);
  }

  template<typename Input>
  void Action<Exit>::apply(const Input &, Data &data) {
    data.productions.top()->actions_or_retractions->push_back(Action_Exit_Generator::Create());
  }

  template<typename Input>
  void Action<Make>::apply(const Input &input, Data &data) {
    //std::cerr << "Make: " << input.string() << std::endl;

    const auto make = std::make_shared<Action_Make_Generator>(data.symbols);
    data.symbols.clear();

    data.productions.top()->actions_or_retractions->push_back(make);
  }

  template<typename Input>
  void Action<Write>::apply(const Input &input, Data &data) {
    //std::cerr << "Write: " << input.string() << std::endl;

    const auto write = std::make_shared<Action_Write_Generator>(data.symbols);
    data.symbols.clear();

    data.productions.top()->actions_or_retractions->push_back(write);
  }

  /// Production Rules

  template<typename Input>
  void Action<Begin_Actions>::apply(const Input &input, Data &data) {
    data.productions.top()->phase = Data::Production::Phase::PHASE_ACTIONS;
    data.productions.top()->actions_or_retractions = &data.productions.top()->actions;
  }

  template<typename Input>
  void Action<Begin_Retractions>::apply(const Input &input, Data &data) {
    data.productions.top()->phase = Data::Production::Phase::PHASE_RETRACTIONS;
    data.productions.top()->actions_or_retractions = &data.productions.top()->retractions;
  }

  template<typename Input>
  void Action<Rule_Name>::apply(const Input &, Data &data) {
    assert(data.symbols.size() == 1);
    data.productions.top()->rule_name = data.symbols.back();
    data.symbols.pop_back();

    data.productions.top()->phase = Data::Production::Phase::PHASE_LHS;
  }

  template<typename Input>
  void Action<Begin_Production>::apply(const Input &, Data &data) {
    data.productions.push(std::make_shared<Data::Production>(data.network, data.job_queue, false, data.productions.top()->lhs_variables));
  }

  template<typename Input>
  void Action<Production>::apply(const Input &input, Data &data) {
    //std::cerr << "Production: " << input.string() << std::endl;

    assert(data.productions.top()->lhs.size() == 1);
    assert(data.productions.top()->lhs.top().size() == 1);
    const auto node = data.productions.top()->lhs.top().top();
    data.productions.top()->lhs.top().pop();

    auto rule_name = data.productions.top()->rule_name;

    const auto action = std::make_shared<Actions_Generator>(data.productions.top()->actions);
    const auto retraction = std::make_shared<Actions_Generator>(data.productions.top()->retractions);

    const auto action_generator = std::make_shared<Node_Action_Generator>(rule_name, node, action, retraction);

    data.productions.pop();

    data.productions.top()->actions_or_retractions->push_back(std::make_shared<Action_Production_Generator>(action_generator));
  }

  /// Overarching grammar

  template<typename Input>
  void Action<Command>::apply(const Input &input, Data &data) {
    assert(data.productions.top()->actions.size() == 1);
    const auto action = data.productions.top()->actions.back()->generate(data.network, data.job_queue, data.productions.top()->user_action, nullptr, nullptr);
    data.productions.top()->actions.pop_back();
    (*action)(data.network, data.job_queue, nullptr, nullptr);
  }

}

#endif
