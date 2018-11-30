#ifndef ZENI_RETE_PARSER_PEG_HPP
#define ZENI_RETE_PARSER_PEG_HPP

// Include these above PEGTL includes to avoid a strange bug in MSVC
#include "Zeni/Concurrency/Container/Antiable_Hash_Trie.hpp"
#include "Zeni/Concurrency/Container/Hash_Trie.hpp"
#include "Zeni/Concurrency/Container/Hash_Trie_S2.hpp"
#include "Zeni/Concurrency/Container/Positive_Hash_Trie.hpp"

#define TAO_PEGTL_NAMESPACE Zeni_Rete_PEG

#include "tao/pegtl.hpp"
#include "tao/pegtl/analyze.hpp"

namespace Zeni::Rete::PEG {

  using namespace tao::Zeni_Rete_PEG;

  struct Data;

  template <typename Rule>
  struct Action : nothing<Rule> {};

  template <typename Rule>
  struct Error : public normal<Rule>
  {
    static inline const char * error_message();

    template <typename Input>
    static void raise(const Input &input, Data &) {
      throw parse_error(error_message(), input);
    }
  };

  /// Basic numerical processing

  struct plus_minus : one<'+', '-'> {};
  struct dot : one<'.'> {};

  struct inf : seq<istring<'i', 'n', 'f'>, opt<istring<'i', 'n', 'i', 't', 'y'>>> {};
  struct nan : seq<istring<'n', 'a', 'n'>, opt<one<'('>, plus<alnum>, one<')'>>> {};

  template<typename D>
  struct number : if_then_else<dot, plus<D>, seq<plus<D>, opt<dot, star<D>>>> {};

  struct e : one<'e', 'E'> {};
  struct p : one<'p', 'P'> {};
  struct exponent : seq<opt<plus_minus>, plus<digit>> {};

  struct decimal : seq<number<digit>, opt<e, exponent>> {};

  /// Comments and spaces

  struct comment : if_must<one<'#'>, until<eolf>> {};
  template<> inline const char * Error<until<eolf>>::error_message() { return "Parser error: failed to process comment"; } ///< Probably cannot be triggered

  struct space_comment : sor<space, comment> {};

  /// Symbols

  struct Unquoted_String : plus<not_one<' ', '\t', '\r', '\n', '|', '<', '>', '@', '(', ')', '{', '}', '='>> {};

  struct Constant_Float : if_then_else<plus_minus, must<sor<decimal, inf, nan>>, sor<decimal, inf, nan>> {};
  template<> inline const char * Error<sor<decimal, inf, nan>>::error_message() { return "Parser error: '+'/'-' must be immediately followed by a numerical value to make a valid symbol"; }
  struct Constant_Int : if_then_else<plus_minus, must<plus<digit>>, plus<digit>> {};
  template<> inline const char * Error<plus<digit>>::error_message() { return "Parser error: '+'/'-' must be immediately followed by a numerical value to make a valid symbol"; }
  struct Constant_String : Unquoted_String {};
  struct Quoted_Constant_String : if_must<one<'|'>, seq<star<not_one<'\r', '\n', '|'>>, one<'|'>>> {};
  template<> inline const char * Error<seq<star<not_one<'\r', '\n', '|'>>, one<'|'>>>::error_message() { return "Parser error: quoted string constant not closed (mismatched '|'s?)"; }
  struct CRLF : seq<one<'('>, star<space_comment>, string<'c', 'r', 'l', 'f'>, star<space_comment>, one<')'>> {};
  struct Identifier_Name : Unquoted_String {};
  struct Constant_Identifier : if_must<one<'@'>, Identifier_Name> {};
  template<> inline const char * Error<Identifier_Name>::error_message() { return "Parser error: '@' must be followed immediately by an identifier name"; }
  template<> inline const char * Error<seq<Unquoted_String, one<'>'>>>::error_message() { return "Parser error: variable not closed (mismatched '<>'s?)"; }
  struct Unnamed_Variable : seq<one<'{'>, star<space_comment>, one<'}'>> {};
  struct Constant_Variable : if_must<one<'<'>, seq<Unquoted_String, one<'>'>>> {};
  struct Bound_Constant_Variable : if_must<one<'<'>, seq<Unquoted_String, one<'>'>>> {};
  struct Unbound_Constant_Variable : if_must<one<'<'>, seq<Unquoted_String, one<'>'>>> {};

  struct Constant_or_Variable : sor<
    seq<at<minus<Constant_Float, Constant_Int>>, Constant_Float>,
    Constant_Int,
    minus<Constant_String, at<sor<Constant_Float, Constant_Int>>>,
    Quoted_Constant_String,
    CRLF,
    Constant_Identifier,
    Constant_Variable> {};

  struct Constant_or_Bound : sor<
    seq<at<minus<Constant_Float, Constant_Int>>, Constant_Float>,
    Constant_Int,
    minus<Constant_String, at<sor<Constant_Float, Constant_Int>>>,
    Quoted_Constant_String,
    CRLF,
    Constant_Identifier,
    Bound_Constant_Variable> {};

  //struct Constant_or_Unbound : sor<
  //  seq<at<minus<Constant_Float, Constant_Int>>, Constant_Float>,
  //  Constant_Int,
  //  minus<Constant_String, at<sor<Constant_Float, Constant_Int>>>,
  //  Quoted_Constant_String,
  //  CRLF,
  //  Constant_Identifier,
  //  Unbound_Constant_Variable,
  //  Unnamed_Variable> {};

  struct Rule_Name : Constant_or_Bound {};

  struct Predicate_E : string<'=', '='> {};
  struct Predicate_NE : string<'!', '='> {};
  struct Predicate_LT : one<'<'> {};
  struct Predicate_LTE : string<'<', '='> {};
  struct Predicate_GT : one<'>'> {};
  struct Predicate_GTE : string<'>', '='> {};
  struct Predicate_STA : string<'<', '=', '>'> {};
  struct Predicate : sor<Predicate_STA, Predicate_E, Predicate_NE, Predicate_LTE, Predicate_GTE, Predicate_GT, Predicate_LT> {};
  struct Predicate_Symbol : seq<star<space_comment>, Constant_or_Bound> {};
  template<> inline const char * Error<Predicate_Symbol>::error_message() { return "Parser error: invalid predicate syntax (missing symbol?)"; }
  struct Predicate_Alpha : if_must<Predicate, Predicate_Symbol> {};

  /// Conditions and WMEs

  struct Disjunction_Value_2 : seq<plus<space_comment>, Constant_or_Bound> {};
  struct Inner_Disjunction : seq<star<space_comment>, Constant_or_Bound, star<Disjunction_Value_2>, star<space_comment>, string<'>', '>'>> {};
  struct Disjunction_End : if_must<string<'<', '<'>, Inner_Disjunction> {};
  template<> inline const char * Error<Inner_Disjunction>::error_message() { return "Parser error: invalid disjunction syntax (mismatched '<<>>'s?)"; }
  struct Disjunction_Begin : at<Disjunction_End> {};
  struct Disjunction : seq<Disjunction_Begin, Disjunction_End> {};

  struct Inner_Conjunction : seq<star<space_comment>, opt<sor<Disjunction, try_catch<Constant_or_Bound>>, star<space_comment>>, star<seq<not_at<try_catch<Unbound_Constant_Variable>>, Predicate_Alpha>, star<space_comment>>, opt<Unbound_Constant_Variable, star<space_comment>>, one<'}'>> {};
  struct Conjunction_End : if_must<one<'{'>, Inner_Conjunction> {};
  template<> inline const char * Error<Inner_Conjunction>::error_message() { return "Parser error: invalid conjunction syntax (mismatched '{}'s or misordered conjunction?)"; }
  struct Conjunction_Begin : at<Conjunction_End> {};
  struct Conjunction : seq<Conjunction_Begin, Conjunction_End> {};

  struct Condition_Value_End : sor<Conjunction, Disjunction, Constant_or_Variable> {};
  struct Condition_Value_Begin : at<Condition_Value_End> {};
  struct Condition_Value : seq<Condition_Value_Begin, Condition_Value_End> {};

  struct Condition_Attr_Value : seq<plus<space_comment>, one<'^'>, star<space_comment>, Condition_Value, plus<space_comment>, Condition_Value> {};
  struct WME_Attr_Value : seq<plus<space_comment>, one<'^'>, star<space_comment>, Constant_or_Bound, plus<space_comment>, Constant_or_Bound> {};

  struct Inner_Condition : seq<star<space_comment>, Condition_Value, plus<Condition_Attr_Value>> {};
  struct Condition_Body : seq<star<space_comment>, Inner_Condition, star<space_comment>, one<')'>, star<space_comment>> {};
  struct Condition_End : if_must<one<'('>, Condition_Body> {};
  template<> inline const char * Error<Condition_Body>::error_message() { return "Parser error: invalid condition syntax (mismatched '()'s?)"; }
  struct Condition_Begin : at<Condition_End> {};
  struct Condition : seq<Condition_Begin, Condition_End> {};

  struct WMEs : seq<star<space_comment>, Constant_or_Bound, plus<WME_Attr_Value>> {};

  /// Left-Hand Side / LHS

  struct Subnode_First;
  struct Subnode_Rest;

  struct Inner_Scope_Body : seq<Subnode_First, star<space_comment>, star<Subnode_Rest, star<space_comment>>> {};
  struct Inner_Scope_End : must<Inner_Scope_Body> {};
  template<> inline const char * Error<Inner_Scope_Body>::error_message() { return "Parser error: invalid left-hand side for production rule"; }
  struct Inner_Scope_Begin : at<Inner_Scope_End> {};
  struct Inner_Scope : seq<Inner_Scope_Begin, Inner_Scope_End> {};
  struct Outer_Scope_Body : seq<Inner_Scope, one<'}'>, star<space_comment>> {};
  struct Outer_Scope : if_must<one<'{'>, Outer_Scope_Body> {};
  template<> inline const char * Error<Outer_Scope_Body>::error_message() { return "Parser error: invalid scope syntax (mismatched '{}'s?)"; }
  struct Scope : seq<sor<Outer_Scope, Condition>, star<space_comment>> {};

  struct Join : Scope {};
  struct EScope : Scope {};
  struct NScope : Scope {};
  struct Join_Existential : if_must<one<'+'>, EScope> {};
  template<> inline const char * Error<EScope>::error_message() { return "Parser error: '+' must be immediately followed by a valid condition or scope"; }
  struct Join_Negation : if_must<seq<not_at<string<'-', '-', '>'>>, one<'-'>>, NScope> {};
  template<> inline const char * Error<NScope>::error_message() { return "Parser error: '-' must be immediately followed by a valid condition or scope"; }

  struct Predicate_Node_Body : seq<plus<space_comment>, Constant_or_Bound, star<space_comment>, one<')'>, star<space_comment>> {};
  struct Predicate_Node_End : if_must<seq<one<'('>, star<space_comment>, Bound_Constant_Variable, plus<space_comment>, Predicate>, Predicate_Node_Body> {};
  template<> inline const char * Error<Predicate_Node_Body>::error_message() { return "Parser error: invalid predicate syntax (mismatched '()'s?)"; }
  struct Predicate_Node_Begin : at<Predicate_Node_End> {};
  struct Predicate_Node : seq<Predicate_Node_Begin, Predicate_Node_End> {};

  struct Subnode_First : Scope {};
  struct Subnode_Rest : sor<Predicate_Node, Join, Join_Existential, Join_Negation> {};

  /// Right-Hand Side / RHS

  struct Inner_Cbind : seq<plus<space_comment>, Unbound_Constant_Variable> {};
  struct Cbind : if_must<string<'c', 'b', 'i', 'n', 'd'>, Inner_Cbind> {};
  template<> inline const char * Error<Inner_Cbind>::error_message() { return "Parser error: 'cbind' must be followed by an unbound variable"; }

  struct Inner_Excise : seq<plus<plus<space_comment>, Constant_or_Bound>> {};
  struct Excise : if_must<string<'e', 'x', 'c', 'i', 's', 'e'>, Inner_Excise> {};
  template<> inline const char * Error<Inner_Excise>::error_message() { return "Parser error: 'excise' must be followed by one or more rule names"; }

  struct Excise_All : string<'e', 'x', 'c', 'i', 's', 'e', '-', 'a', 'l', 'l'> {};

  struct Exit : string<'e', 'x', 'i', 't'> {};

  struct Genatom : string<'g', 'e', 'n', 'a', 't', 'o', 'm'> {};

  struct Inner_Make : seq<star<space_comment>, WMEs> {};
  struct Make : if_must<string<'m', 'a', 'k', 'e'>, Inner_Make> {};
  template<> inline const char * Error<Inner_Make>::error_message() { return "Parser error: 'make' must be followed by a valid WME"; }

  struct Inner_Production_Body;
  struct Production_Body : seq<plus<space_comment>, Inner_Production_Body> {};
  struct End_Production : if_must<one<'p'>, Production_Body> {};
  template<> inline const char * Error<Production_Body>::error_message() { return "Parser error: 'p' must be followed by a valid production rule body"; }
  struct Begin_Production : at<End_Production> {};
  struct Production : seq<Begin_Production, End_Production> {};

  struct Inner_Remove : seq<star<space_comment>, WMEs> {};
  struct Remove : if_must<string<'r', 'e', 'm', 'o', 'v', 'e'>, Inner_Remove> {};
  template<> inline const char * Error<Inner_Remove>::error_message() { return "Parser error: 'remove' must be followed by a valid WME"; }

  struct Inner_Source : plus<plus<space_comment>, Constant_or_Bound> {};
  struct Source : if_must<string<'s', 'o', 'u', 'r', 'c', 'e'>, Inner_Source> {};
  template<> inline const char * Error<Inner_Source>::error_message() { return "Parser error: 'source' must be followed by one or more valid file names."; }

  struct Inner_Write : plus<plus<space_comment>, Constant_or_Bound> {};
  struct Write : if_must<string<'w', 'r', 'i', 't', 'e'>, Inner_Write> {};
  template<> inline const char * Error<Inner_Write>::error_message() { return "Parser error: 'write' must be followed by valid Symbols"; }

  /// Production Rules

  struct Inner_Action : sor<Cbind, Excise_All, Excise, Exit, Genatom, Make, Production, Remove, Source, Write> {};

  struct Enclosed_Action : seq<one<'('>, star<space_comment>, Inner_Action, star<space_comment>, one<')'>, star<space_comment>> {};
  struct Action_List : plus<Enclosed_Action> {};
  struct Inner_Actions : seq<string<'-', '-', '>'>, star<space_comment>, Action_List> {};
  struct Begin_Actions : at<Inner_Actions> {};
  struct Actions : seq<Begin_Actions, Inner_Actions> {};
  struct Inner_Retractions : seq<string<'<', '-', '-'>, star<space_comment>, Action_List> {};
  struct Begin_Retractions : at<Inner_Retractions> {};
  struct Retractions : seq<Begin_Retractions, Inner_Retractions> {};
  struct Inner_Production_Body :
    seq<Rule_Name, star<space_comment>,
    Inner_Scope, star<space_comment>,
    Actions, star<space_comment>,
    opt<seq<Retractions, star<space_comment>>>> {};

  /// Overarching grammar

  struct Inner_Top_Level_Action : sor<Excise_All, Excise, Exit, Make, Production, Remove, Source, Write> {};
  struct Enclosed_Top_Level_Action : seq<one<'('>, star<space_comment>, must<Inner_Top_Level_Action>, star<space_comment>, one<')'>, star<space_comment>> {};
  template<> inline const char * Error<Inner_Top_Level_Action>::error_message() { return "Parser error: invalid top level action"; }
  struct Command : Enclosed_Top_Level_Action {};

  struct ConcRete_Grammar : seq<star<space_comment>, star<seq<Command, star<space_comment>>>, eof> {};

  struct ConcRete : must<ConcRete_Grammar> {};
  template<> inline const char * Error<ConcRete_Grammar>::error_message() { return "Parser error: could not process ConcRete grammar"; }

}

#endif
