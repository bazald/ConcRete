#ifndef ZENI_RETE_PARSER_ACTIONS_HPP
#define ZENI_RETE_PARSER_ACTIONS_HPP

#include "PEG.hpp"

namespace Zeni::Rete::PEG {

  /// Symbols

  template <>
  struct Action<Constant_Float> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Constant_Int> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Constant_String> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Quoted_Constant_String> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<CRLF> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Constant_Identifier> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Constant_Variable> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Bound_Constant_Variable> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<External_Constant_Variable> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Unbound_Constant_Variable> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Unnamed_Variable> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  /// Predicates

  template <>
  struct Action<Predicate_E> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Predicate_NE> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Predicate_LT> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Predicate_LTE> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Predicate_GT> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Predicate_GTE> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Predicate_STA> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Predicate_Symbol> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  /// Conditions and WMEs

  template <>
  struct Action<Condition_Value_Begin> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Condition_Begin> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Condition_Attr_Value> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Condition_End> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  /// Left-Hand Side / LHS

  template <>
  struct Action<Inner_Scope_Begin> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Inner_Scope_End> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Join> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Join_Existential> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Join_Negation> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Predicate_Node> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  /// Math for RHS

  template <>
  struct Action<Constant_Number_or_Bound> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Math_Operator> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Math_Expression_Extension> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Math_Expression> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Math_Parenthesis_Begin> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Math_Parenthesis_End> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Final_Math_Expression> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  /// Right-Hand Side / RHS

  template <>
  struct Action<Cbind> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Excise> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Excise_All> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Exit> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Genatom> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Make> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Remove> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Source> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Write> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Inner_Action> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  /// Production Rules

  template <>
  struct Action<Action_List> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Begin_Actions> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Begin_Retractions> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Rule_Name> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Begin_Production> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<Production> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  /// Overarching grammar

  template <>
  struct Action<Command> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

}

#endif
