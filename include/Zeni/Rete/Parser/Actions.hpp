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
  struct Action<Unnamed_Variable> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  template <>
  struct Action<RHS_Constant_Variable> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  /// Conditions and WMEs

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
  struct Action<Write> {
    template<typename Input>
    static void apply(const Input &input, Data &data);
  };

  /// Production Rules

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
