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
    data.symbols.top()->symbols.emplace_back(std::make_shared<Symbol_Constant_Generator>(symbol));
  }

  template<typename Input>
  void Action<Constant_Int>::apply(const Input &input, Data &data) {
    const int64_t i = std::stoll(input.string());
    //std::cerr << "Constant_Int: " << i << std::endl;
    const auto symbol = std::make_shared<Symbol_Constant_Int>(i);
    data.symbols.top()->symbols.emplace_back(std::make_shared<Symbol_Constant_Generator>(symbol));
  }

  template<typename Input>
  void Action<Constant_String>::apply(const Input &input, Data &data) {
    //std::cerr << "Constant_String: " << input.string() << std::endl;
    const auto symbol = std::make_shared<Symbol_Constant_String>(input.string());
    data.symbols.top()->symbols.emplace_back(std::make_shared<Symbol_Constant_Generator>(symbol));
  }

  template<typename Input>
  void Action<Quoted_Constant_String>::apply(const Input &input, Data &data) {
    //std::cerr << "Quoted_Constant_String: " << input.string() << std::endl;
    assert(input.string().size() > 2);
    const auto substr = input.string().substr(1, input.string().size() - 2);
    const auto symbol = std::make_shared<Symbol_Constant_String>(substr);
    data.symbols.top()->symbols.emplace_back(std::make_shared<Symbol_Constant_Generator>(symbol));
  }

  template<typename Input>
  void Action<CRLF>::apply(const Input &input, Data &data) {
    //std::cerr << "CRLF: " << input.string() << std::endl;
    const auto symbol = std::make_shared<Symbol_Constant_String>("\r\n");
    data.symbols.top()->symbols.emplace_back(std::make_shared<Symbol_Constant_Generator>(symbol));
  }

  template<typename Input>
  void Action<Constant_Identifier>::apply(const Input &input, Data &data) {
    //std::cerr << "Identifier: " << input.string() << std::endl;
    assert(input.string().size() > 1);
    const auto substr = input.string().substr(1, input.string().size() - 1);
    const auto symbol = std::make_shared<Symbol_Constant_Identifier>(substr);
    data.symbols.top()->symbols.emplace_back(std::make_shared<Symbol_Constant_Generator>(symbol));
  }

  template<typename Input>
  void Action<Constant_Variable>::apply(const Input &input, Data &data) {
    //std::cerr << "Constant_Variable: " << input.string() << std::endl;
    assert(input.string().size() > 2);
    const auto substr = input.string().substr(1, input.string().size() - 2);
    const auto symbol = std::make_shared<Symbol_Variable>(substr.c_str());
    if (data.productions.top()->external_lhs_variables->find(substr) != data.productions.top()->external_lhs_variables->end()) {
      /// External variable should be treated as constant
      data.symbols.top()->symbols.emplace_back(std::make_shared<Symbol_Variable_Generator>(symbol));
    }
    else {
      /// Internal variable must be treated as new binding
      data.symbols.top()->variable = symbol;
      data.productions.top()->lhs_variables->insert(substr);
    }
  }

  template<typename Input>
  void Action<Bound_Constant_Variable>::apply(const Input &input, Data &data) {
    //std::cerr << "Bound_Constant_Variable: " << input.string() << std::endl;
    assert(input.string().size() > 2);
    const auto substr = input.string().substr(1, input.string().size() - 2);
    if (data.productions.top()->lhs_variables->find(substr) == data.productions.top()->lhs_variables->end())
      throw parse_error("Parser error: use of unbound variable", input);
    data.productions.top()->lhs_variables->insert(substr);
    const auto symbol = std::make_shared<Symbol_Variable>(substr.c_str());
    data.symbols.top()->symbols.emplace_back(std::make_shared<Symbol_Variable_Generator>(symbol));
  }

  template<typename Input>
  void Action<External_Constant_Variable>::apply(const Input &input, Data &data) {
    //std::cerr << "External_Constant_Variable: " << input.string() << std::endl;
    assert(input.string().size() > 2);
    const auto substr = input.string().substr(1, input.string().size() - 2);
    if (data.productions.top()->external_lhs_variables->find(substr) == data.productions.top()->external_lhs_variables->end())
      throw parse_error("Parser error: use of unbound or internally bound variable", input);
    data.productions.top()->lhs_variables->insert(substr);
    const auto symbol = std::make_shared<Symbol_Variable>(substr.c_str());
    data.symbols.top()->symbols.emplace_back(std::make_shared<Symbol_Variable_Generator>(symbol));
  }

  template<typename Input>
  void Action<Unbound_Constant_Variable>::apply(const Input &input, Data &data) {
    //std::cerr << "Unbound_Constant_Variable: " << input.string() << std::endl;
    assert(input.string().size() > 2);
    const auto substr = input.string().substr(1, input.string().size() - 2);
    if (data.productions.top()->lhs_variables->find(substr) != data.productions.top()->lhs_variables->end())
      throw parse_error("Parser error: variable previously bound", input);
    data.productions.top()->lhs_variables->insert(substr);
    const auto symbol = std::make_shared<Symbol_Variable>(substr.c_str());
    data.symbols.top()->variable = symbol;
  }

  template<typename Input>
  void Action<Unnamed_Variable>::apply(const Input &input, Data &data) {
    //std::cerr << "Unnamed_Variable: " << input.string() << std::endl;
    const auto symbol = std::make_shared<Symbol_Variable>("");
    data.symbols.top()->symbols.emplace_back(std::make_shared<Symbol_Variable_Generator>(symbol));
  }

  /// Predicates

  template<typename Input>
  void Action<Predicate_E>::apply(const Input &input, Data &data) {
    data.symbols.top()->predicates.emplace_back(Node_Predicate::Predicate_E::Create(), nullptr);
  }

  template<typename Input>
  void Action<Predicate_NE>::apply(const Input &input, Data &data) {
    data.symbols.top()->predicates.emplace_back(Node_Predicate::Predicate_NE::Create(), nullptr);
  }

  template<typename Input>
  void Action<Predicate_LT>::apply(const Input &input, Data &data) {
    data.symbols.top()->predicates.emplace_back(Node_Predicate::Predicate_LT::Create(), nullptr);
  }

  template<typename Input>
  void Action<Predicate_LTE>::apply(const Input &input, Data &data) {
    data.symbols.top()->predicates.emplace_back(Node_Predicate::Predicate_LTE::Create(), nullptr);
  }

  template<typename Input>
  void Action<Predicate_GT>::apply(const Input &input, Data &data) {
    data.symbols.top()->predicates.emplace_back(Node_Predicate::Predicate_GT::Create(), nullptr);
  }

  template<typename Input>
  void Action<Predicate_GTE>::apply(const Input &input, Data &data) {
    data.symbols.top()->predicates.emplace_back(Node_Predicate::Predicate_GTE::Create(), nullptr);
  }

  template<typename Input>
  void Action<Predicate_STA>::apply(const Input &input, Data &data) {
    data.symbols.top()->predicates.emplace_back(Node_Predicate::Predicate_STA::Create(), nullptr);
  }

  template<typename Input>
  void Action<Predicate_Symbol>::apply(const Input &input, Data &data) {
    data.symbols.top()->predicates.back().second = std::move(data.symbols.top()->symbols.back());
    data.symbols.top()->symbols.pop_back();
  }

  /// Conditions and WMEs

  template<typename Input>
  void Action<Condition_Value_Begin>::apply(const Input &input, Data &data) {
    //std::cerr << "Condition_Value_Begin: " << input.string() << std::endl;

    data.symbols.push(std::make_shared<Symbols>());
  }

  template<typename Input>
  void Action<Condition_Begin>::apply(const Input &input, Data &data) {
    //std::cerr << "Condition_Begin: " << input.string() << std::endl;

    data.productions.top()->lhs.emplace(typename decltype(data.productions.top()->lhs)::value_type());
  }

  template<typename Input>
  void Action<Condition_Attr_Value>::apply(const Input &input, Data &data) {
    //std::cerr << "Condition_Attr_Value: " << input.string() << std::endl;

    assert(data.symbols.size() == 4);
    const auto third = std::move(data.symbols.top());
    data.symbols.pop();
    const auto second = std::move(data.symbols.top());
    data.symbols.pop();
    const auto first = data.symbols.top();
    // Deliberately leave the first symbols on the stack

    if (first->symbols.empty() && second->symbols.empty() && third->symbols.empty())
      throw parse_error("Parser error: a condition must include at least one non-variable", input);

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

    assert(data.symbols.size() == 2);
    data.symbols.pop();

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

  template<typename Input>
  void Action<Predicate_Node>::apply(const Input &input, Data &data) {
    assert(data.symbols.top()->predicates.size() == 1);
    assert(!data.symbols.top()->predicates.front().second);
    assert(data.symbols.top()->symbols.size() == 2);
    const auto pred = data.symbols.top()->predicates.front().first;
    data.symbols.top()->predicates.clear();
    const auto lhs = std::dynamic_pointer_cast<const Symbol_Variable_Generator>(data.symbols.top()->symbols.front());
    assert(lhs);
    const auto rhs = data.symbols.top()->symbols.back();
    data.symbols.top()->symbols.clear();

    const auto node = data.productions.top()->lhs.top().top();
    data.productions.top()->lhs.top().pop();
    data.productions.top()->lhs.top().push(std::make_shared<Node_Predicate_Generator>(node, pred, lhs, rhs));
  }

  /// Math for RHS

  template<typename Input>
  void Action<Constant_Number_or_Bound>::apply(const Input &input, Data &data) {
    //std::cerr << "Constant_Number_or_Bound: " << input.string() << std::endl;

    const auto symbol = data.symbols.top()->symbols.back();
    data.symbols.top()->symbols.pop_back();

    data.symbols.top()->math.top().maths.push_back(std::make_shared<Math_Constant_Generator>(symbol));
  }

  template<typename Input>
  void Action<Math_Operator>::apply(const Input &input, Data &data) {
    //std::cerr << "Math_Operator: " << input.string() << std::endl;

    data.symbols.top()->math.top().math_operators.push_back(input.string().at(0));
  }

  template<typename Input>
  void Action<Math_Expression_Extension>::apply(const Input &input, Data &data) {
    //std::cerr << "Math_Expression_Extension: " << input.string() << std::endl;

    assert(!data.symbols.top()->math.top().math_operators.empty());
    assert(data.symbols.top()->math.top().maths.size() == data.symbols.top()->math.top().math_operators.size() + 1);

    switch (const char oc = data.symbols.top()->math.top().math_operators.back()) {
    case '*':
    case '/':
    case '%':
    {
      const auto second = data.symbols.top()->math.top().maths.back();
      data.symbols.top()->math.top().maths.pop_back();
      const auto first = data.symbols.top()->math.top().maths.back();
      data.symbols.top()->math.top().maths.pop_back();

      switch (oc) {
      case '*':
        data.symbols.top()->math.top().maths.push_back(Math_Product_Generator::Create(first, second));
        break;

      case '/':
        data.symbols.top()->math.top().maths.push_back(Math_Quotient_Generator::Create(first, second));
        break;

      case '%':
        data.symbols.top()->math.top().maths.push_back(Math_Remainder_Generator::Create(first, second));
        break;

      default:
        abort();
      }

      data.symbols.top()->math.top().math_operators.pop_back();

      break;
    }

    default:
      break;
    }
  }

  template<typename Input>
  void Action<Math_Expression>::apply(const Input &input, Data &data) {
    //std::cerr << "Math_Expression: " << input.string() << std::endl;

    assert(data.symbols.top()->math.top().maths.size() == data.symbols.top()->math.top().math_operators.size() + 1);

    auto mt = data.symbols.top()->math.top().maths.cbegin();
    const auto mend = data.symbols.top()->math.top().maths.cend();
    auto value = *mt++;

    auto ot = data.symbols.top()->math.top().math_operators.cbegin();
    //const auto oend = data.symbols.top()->math.top().math_operators.cend();

    while (mt != mend) {
      switch (*ot) {
      case '+':
        value = Math_Sum_Generator::Create(value, *mt);
        break;

      case '-':
        value = Math_Difference_Generator::Create(value, *mt);
        break;

      default:
        abort();
      }

      ++mt, ++ot;
    }

    data.symbols.top()->math.top().maths.clear();
    data.symbols.top()->math.top().math_operators.clear();
    data.symbols.top()->math.top().maths.push_back(value);
  }

  template<typename Input>
  void Action<Math_Parenthesis_Begin>::apply(const Input &input, Data &data) {
    //std::cerr << "Math_Parenthesis_Begin: " << input.string() << std::endl;
    
    data.symbols.top()->math.push(Symbols::Math());
  }

  template<typename Input>
  void Action<Math_Parenthesis_End>::apply(const Input &input, Data &data) {
    //std::cerr << "Math_Parenthesis_End: " << input.string() << std::endl;

    assert(data.symbols.top()->math.size() > 1);
    assert(data.symbols.top()->math.top().maths.size() == 1);
    assert(data.symbols.top()->math.top().math_operators.empty());

    const auto value = data.symbols.top()->math.top().maths.back();
    data.symbols.top()->math.pop();
    data.symbols.top()->math.top().maths.push_back(value);
  }

  template<typename Input>
  void Action<Final_Math_Expression>::apply(const Input &input, Data &data) {
    //std::cerr << "Final_Math_Expression: " << input.string() << std::endl;

    assert(data.symbols.size() == 1);
    assert(data.symbols.top()->math.top().maths.size() == 1);
    assert(data.symbols.top()->math.top().math_operators.empty());

    data.productions.top()->actions_or_retractions->push_back(data.symbols.top()->math.top().maths.back());
    data.symbols.top()->math.top().maths.clear();
  }

  /// Right-Hand Side / RHS

  template<typename Input>
  void Action<Cbind>::apply(const Input &input, Data &data) {
    //std::cerr << "Cbind: " << input.string() << std::endl;

    assert(data.symbols.size() == 1);
    assert(data.symbols.top()->variable);
    const auto cbind = std::make_shared<Action_Cbind_Generator>(std::move(data.symbols.top()->variable));
    data.symbols.top()->variable.reset();

    data.productions.top()->actions_or_retractions->push_back(cbind);
  }

  template<typename Input>
  void Action<Excise>::apply(const Input &input, Data &data) {
    //std::cerr << "Excise: " << input.string() << std::endl;

    assert(data.symbols.size() == 1);
    const auto excise = std::make_shared<Action_Excise_Generator>(std::move(data.symbols.top()->symbols));
    data.symbols.top()->symbols.clear();

    data.productions.top()->actions_or_retractions->push_back(excise);
  }

  template<typename Input>
  void Action<Excise_All>::apply(const Input &input, Data &data) {
    //std::cerr << "Excise_All: " << input.string() << std::endl;

    const auto excise_all = Action_Excise_All_Generator::Create();

    data.productions.top()->actions_or_retractions->push_back(excise_all);
  }

  template<typename Input>
  void Action<Exit>::apply(const Input &, Data &data) {
    data.productions.top()->actions_or_retractions->push_back(Action_Exit_Generator::Create());
  }

  template<typename Input>
  void Action<Genatom>::apply(const Input &, Data &data) {
    data.productions.top()->actions_or_retractions->push_back(Action_Genatom_Generator::Create());
  }

  template<typename Input>
  void Action<Make>::apply(const Input &input, Data &data) {
    //std::cerr << "Make: " << input.string() << std::endl;

    assert(data.symbols.size() == 1);
    const auto make = std::make_shared<Action_Make_Generator>(std::move(data.symbols.top()->symbols));
    data.symbols.top()->symbols.clear();

    data.productions.top()->actions_or_retractions->push_back(make);
  }

  template<typename Input>
  void Action<Remove>::apply(const Input &input, Data &data) {
    //std::cerr << "Remove: " << input.string() << std::endl;

    assert(data.symbols.size() == 1);
    const auto remove = std::make_shared<Action_Remove_Generator>(std::move(data.symbols.top()->symbols));
    data.symbols.top()->symbols.clear();

    data.productions.top()->actions_or_retractions->push_back(remove);
  }

  template<typename Input>
  void Action<Source>::apply(const Input &input, Data &data) {
    //std::cerr << "Source: " << input.string() << std::endl;

    assert(data.symbols.size() == 1);
    const auto source = std::make_shared<Action_Source_Generator>(std::move(data.symbols.top()->symbols));
    data.symbols.top()->symbols.clear();

    data.productions.top()->actions_or_retractions->push_back(source);
  }

  template<typename Input>
  void Action<Write>::apply(const Input &input, Data &data) {
    //std::cerr << "Write: " << input.string() << std::endl;

    assert(data.symbols.size() == 1);
    const auto write = std::make_shared<Action_Write_Generator>(std::move(data.symbols.top()->symbols));
    data.symbols.top()->symbols.clear();

    data.productions.top()->actions_or_retractions->push_back(write);
  }

  template<typename Input>
  void Action<Inner_Action>::apply(const Input &input, Data &data) {
    //std::cerr << "Inner_Action: " << input.string() << std::endl;

    const auto result = data.productions.top()->actions_or_retractions->empty() ? Action_Generator::Result::RESULT_UNTOUCHED : data.productions.top()->actions_or_retractions->back()->result();
    if (result & Action_Generator::Result::RESULT_CONSUMED) {
      if (!data.productions.top()->result_available)
        throw parse_error("Parser error: no result available for action to consume", input);
      data.productions.top()->result_available = result & Action_Generator::Result::RESULT_PROVIDED;
      data.productions.top()->result_must_be_consumed = result & Action_Generator::Result::RESULT_MUST_BE_CONSUMED;
    }
    else if (result & Action_Generator::Result::RESULT_PROVIDED) {
      if (data.productions.top()->result_must_be_consumed)
        throw parse_error("Parser error: result that must be consumed is ignored", input);
      data.productions.top()->result_available = true;
      data.productions.top()->result_must_be_consumed = result & Action_Generator::Result::RESULT_MUST_BE_CONSUMED;
    }
  }

  /// Production Rules

  template<typename Input>
  void Action<Action_List>::apply(const Input &input, Data &data) {
    if (data.productions.top()->result_must_be_consumed)
      throw parse_error("Parser error: result that must be consumed is ignored", input);
    data.productions.top()->result_available = false;
  }

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
    assert(data.symbols.top()->symbols.size() == 1);
    data.productions.top()->rule_name = data.symbols.top()->symbols.back();
    data.symbols.top()->symbols.pop_back();

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
    const auto action = data.productions.top()->actions.back()->generate(data.network, data.job_queue, data.productions.top()->user_action, nullptr);
    data.productions.top()->actions.pop_back();
    (*action)(data.network, data.job_queue, nullptr, nullptr);
  }

}

#endif
