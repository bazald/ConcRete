#ifndef ZENI_RETE_SYMBOL_HPP
#define ZENI_RETE_SYMBOL_HPP

#include "Internal/Linkage.hpp"

#include <memory>
#include <string>

namespace Zeni::Rete {
  class Symbol;
}

namespace std {
  template class ZENI_RETE_LINKAGE std::shared_ptr<const Zeni::Rete::Symbol>;
}

namespace Zeni::Rete {

  class Symbol_Constant;
  class Symbol_Constant_Float;
  class Symbol_Constant_Int;
  class Symbol_Constant_String;
  class Symbol_Identifier;
  class Symbol_Variable;
  class Variable_Indices;

  class Symbol
  {
    Symbol(const Symbol &) = delete;
    Symbol & operator=(const Symbol &) = delete;

  public:
    ZENI_RETE_LINKAGE Symbol();
    ZENI_RETE_LINKAGE virtual ~Symbol();

    ZENI_RETE_LINKAGE virtual Symbol * clone() const = 0;

    ZENI_RETE_LINKAGE virtual bool operator==(const Symbol &rhs) const = 0;
    ZENI_RETE_LINKAGE virtual bool operator!=(const Symbol &rhs) const = 0;
    ZENI_RETE_LINKAGE virtual bool operator<(const Symbol &rhs) const = 0;
    ZENI_RETE_LINKAGE virtual bool operator<=(const Symbol &rhs) const = 0;
    ZENI_RETE_LINKAGE virtual bool operator>(const Symbol &rhs) const = 0;
    ZENI_RETE_LINKAGE virtual bool operator>=(const Symbol &rhs) const = 0;

    ZENI_RETE_LINKAGE virtual bool operator==(const Symbol_Constant_Float &) const;
    ZENI_RETE_LINKAGE virtual bool operator!=(const Symbol_Constant_Float &) const;
    ZENI_RETE_LINKAGE virtual bool operator<(const Symbol_Constant_Float &) const;
    ZENI_RETE_LINKAGE virtual bool operator<=(const Symbol_Constant_Float &) const;
    ZENI_RETE_LINKAGE virtual bool operator>(const Symbol_Constant_Float &) const;
    ZENI_RETE_LINKAGE virtual bool operator>=(const Symbol_Constant_Float &) const;

    ZENI_RETE_LINKAGE virtual bool operator==(const Symbol_Constant_Int &) const;
    ZENI_RETE_LINKAGE virtual bool operator!=(const Symbol_Constant_Int &) const;
    ZENI_RETE_LINKAGE virtual bool operator<(const Symbol_Constant_Int &) const;
    ZENI_RETE_LINKAGE virtual bool operator<=(const Symbol_Constant_Int &) const;
    ZENI_RETE_LINKAGE virtual bool operator>(const Symbol_Constant_Int &) const;
    ZENI_RETE_LINKAGE virtual bool operator>=(const Symbol_Constant_Int &) const;

    ZENI_RETE_LINKAGE virtual bool operator==(const Symbol_Constant_String &) const;
    ZENI_RETE_LINKAGE virtual bool operator!=(const Symbol_Constant_String &) const;
    ZENI_RETE_LINKAGE virtual bool operator<(const Symbol_Constant_String &) const;
    ZENI_RETE_LINKAGE virtual bool operator<=(const Symbol_Constant_String &) const;
    ZENI_RETE_LINKAGE virtual bool operator>(const Symbol_Constant_String &) const;
    ZENI_RETE_LINKAGE virtual bool operator>=(const Symbol_Constant_String &) const;

    ZENI_RETE_LINKAGE virtual bool operator==(const Symbol_Identifier &) const;
    ZENI_RETE_LINKAGE virtual bool operator!=(const Symbol_Identifier &) const;
    ZENI_RETE_LINKAGE virtual bool operator<(const Symbol_Identifier &) const;
    ZENI_RETE_LINKAGE virtual bool operator<=(const Symbol_Identifier &) const;
    ZENI_RETE_LINKAGE virtual bool operator>(const Symbol_Identifier &) const;
    ZENI_RETE_LINKAGE virtual bool operator>=(const Symbol_Identifier &) const;

    ZENI_RETE_LINKAGE virtual bool operator==(const Symbol_Variable &) const;
    ZENI_RETE_LINKAGE virtual bool operator!=(const Symbol_Variable &) const;
    ZENI_RETE_LINKAGE virtual bool operator<(const Symbol_Variable &) const;
    ZENI_RETE_LINKAGE virtual bool operator<=(const Symbol_Variable &) const;
    ZENI_RETE_LINKAGE virtual bool operator>(const Symbol_Variable &) const;
    ZENI_RETE_LINKAGE virtual bool operator>=(const Symbol_Variable &) const;

    ZENI_RETE_LINKAGE virtual bool operator==(const double) const;
    ZENI_RETE_LINKAGE virtual bool operator==(const int64_t) const;
    ZENI_RETE_LINKAGE virtual bool operator==(const char *) const;

    ZENI_RETE_LINKAGE virtual size_t hash() const = 0;
    ZENI_RETE_LINKAGE virtual std::ostream & print(std::ostream &os) const = 0;
    ZENI_RETE_LINKAGE virtual std::ostream & print(std::ostream &os, const std::shared_ptr<const Variable_Indices> indices) const = 0;
  };

  class Symbol_Constant : public Symbol {
    Symbol_Constant(const Symbol_Constant &) = delete;
    Symbol_Constant & operator=(const Symbol_Constant &) = delete;

  public:
    ZENI_RETE_LINKAGE Symbol_Constant();
  };

  class Symbol_Constant_Float : public Symbol_Constant {
    Symbol_Constant_Float(const Symbol_Constant_Float &) = delete;
    Symbol_Constant_Float & operator=(const Symbol_Constant_Float &) = delete;

  public:
    ZENI_RETE_LINKAGE Symbol_Constant_Float(const double &value_);

    ZENI_RETE_LINKAGE Symbol_Constant_Float * clone() const override;

    ZENI_RETE_LINKAGE bool operator==(const Symbol &rhs) const override;
    ZENI_RETE_LINKAGE bool operator!=(const Symbol &rhs) const override;
    ZENI_RETE_LINKAGE bool operator>(const Symbol &rhs) const override;
    ZENI_RETE_LINKAGE bool operator>=(const Symbol &rhs) const override;
    ZENI_RETE_LINKAGE bool operator<(const Symbol &rhs) const override;
    ZENI_RETE_LINKAGE bool operator<=(const Symbol &rhs) const override;

    ZENI_RETE_LINKAGE bool operator==(const Symbol_Constant_Float &rhs) const override;
    ZENI_RETE_LINKAGE bool operator!=(const Symbol_Constant_Float &rhs) const override;
    ZENI_RETE_LINKAGE bool operator<(const Symbol_Constant_Float &rhs) const override;
    ZENI_RETE_LINKAGE bool operator<=(const Symbol_Constant_Float &rhs) const override;
    ZENI_RETE_LINKAGE bool operator>(const Symbol_Constant_Float &rhs) const override;
    ZENI_RETE_LINKAGE bool operator>=(const Symbol_Constant_Float &rhs) const override;

    ZENI_RETE_LINKAGE bool operator==(const Symbol_Constant_Int &rhs) const override;
    ZENI_RETE_LINKAGE bool operator!=(const Symbol_Constant_Int &rhs) const override;
    ZENI_RETE_LINKAGE bool operator<(const Symbol_Constant_Int &rhs) const override;
    ZENI_RETE_LINKAGE bool operator<=(const Symbol_Constant_Int &rhs) const override;
    ZENI_RETE_LINKAGE bool operator>(const Symbol_Constant_Int &rhs) const override;
    ZENI_RETE_LINKAGE bool operator>=(const Symbol_Constant_Int &rhs) const override;

    ZENI_RETE_LINKAGE bool operator<(const Symbol_Constant_String &) const override;
    ZENI_RETE_LINKAGE bool operator<=(const Symbol_Constant_String &) const override;
    ZENI_RETE_LINKAGE bool operator<(const Symbol_Identifier &) const override;
    ZENI_RETE_LINKAGE bool operator<=(const Symbol_Identifier &) const override;
    ZENI_RETE_LINKAGE bool operator<(const Symbol_Variable &) const override;
    ZENI_RETE_LINKAGE bool operator<=(const Symbol_Variable &) const override;

    ZENI_RETE_LINKAGE bool operator==(const double value_) const override;

    ZENI_RETE_LINKAGE size_t hash() const override;

    ZENI_RETE_LINKAGE virtual std::ostream & print(std::ostream &os) const override;
      
    ZENI_RETE_LINKAGE virtual std::ostream & print(std::ostream &os, const std::shared_ptr<const Variable_Indices>) const override;

    const double value;
  };

  class Symbol_Constant_Int : public Symbol_Constant {
    Symbol_Constant_Int(const Symbol_Constant_Int &) = delete;
    Symbol_Constant_Int & operator=(const Symbol_Constant_Int &) = delete;

  public:
    ZENI_RETE_LINKAGE Symbol_Constant_Int(const int64_t &value_);

    ZENI_RETE_LINKAGE Symbol_Constant_Int * clone() const override;

    ZENI_RETE_LINKAGE bool operator==(const Symbol &rhs) const override;
    ZENI_RETE_LINKAGE bool operator!=(const Symbol &rhs) const override;
    ZENI_RETE_LINKAGE bool operator>(const Symbol &rhs) const override;
    ZENI_RETE_LINKAGE bool operator>=(const Symbol &rhs) const override;
    ZENI_RETE_LINKAGE bool operator<(const Symbol &rhs) const override;
    ZENI_RETE_LINKAGE bool operator<=(const Symbol &rhs) const override;

    ZENI_RETE_LINKAGE bool operator==(const Symbol_Constant_Float &rhs) const override;
    ZENI_RETE_LINKAGE bool operator!=(const Symbol_Constant_Float &rhs) const override;
    ZENI_RETE_LINKAGE bool operator<(const Symbol_Constant_Float &rhs) const override;
    ZENI_RETE_LINKAGE bool operator<=(const Symbol_Constant_Float &rhs) const override;
    ZENI_RETE_LINKAGE bool operator>(const Symbol_Constant_Float &rhs) const override;
    ZENI_RETE_LINKAGE bool operator>=(const Symbol_Constant_Float &rhs) const override;

    ZENI_RETE_LINKAGE bool operator==(const Symbol_Constant_Int &rhs) const override;
    ZENI_RETE_LINKAGE bool operator!=(const Symbol_Constant_Int &rhs) const override;
    ZENI_RETE_LINKAGE bool operator<(const Symbol_Constant_Int &rhs) const override;
    ZENI_RETE_LINKAGE bool operator<=(const Symbol_Constant_Int &rhs) const override;
    ZENI_RETE_LINKAGE bool operator>(const Symbol_Constant_Int &rhs) const override;
    ZENI_RETE_LINKAGE bool operator>=(const Symbol_Constant_Int &rhs) const override;

    ZENI_RETE_LINKAGE bool operator<(const Symbol_Constant_String &) const override;
    ZENI_RETE_LINKAGE bool operator<=(const Symbol_Constant_String &) const override;
    ZENI_RETE_LINKAGE bool operator<(const Symbol_Identifier &) const override;
    ZENI_RETE_LINKAGE bool operator<=(const Symbol_Identifier &) const override;
    ZENI_RETE_LINKAGE bool operator<(const Symbol_Variable &) const override;
    ZENI_RETE_LINKAGE bool operator<=(const Symbol_Variable &) const override;

    ZENI_RETE_LINKAGE bool operator==(const int64_t value_) const override;

    ZENI_RETE_LINKAGE size_t hash() const override;

    ZENI_RETE_LINKAGE virtual std::ostream & print(std::ostream &os) const override;

    ZENI_RETE_LINKAGE virtual std::ostream & print(std::ostream &os, const std::shared_ptr<const Variable_Indices>) const override;

    const int64_t value;
  };

  class Symbol_Constant_String : public Symbol_Constant {
    Symbol_Constant_String(const Symbol_Constant_String &) = delete;
    Symbol_Constant_String & operator=(const Symbol_Constant_String &) = delete;

  public:
    ZENI_RETE_LINKAGE Symbol_Constant_String(const std::string &value_);
    ZENI_RETE_LINKAGE Symbol_Constant_String(std::string &&value_);

    ZENI_RETE_LINKAGE std::string_view get_value() const;

    ZENI_RETE_LINKAGE Symbol_Constant_String * clone() const override;

    ZENI_RETE_LINKAGE bool operator==(const Symbol &rhs) const override;
    ZENI_RETE_LINKAGE bool operator!=(const Symbol &rhs) const override;
    ZENI_RETE_LINKAGE bool operator>(const Symbol &rhs) const override;
    ZENI_RETE_LINKAGE bool operator>=(const Symbol &rhs) const override;
    ZENI_RETE_LINKAGE bool operator<(const Symbol &rhs) const override;
    ZENI_RETE_LINKAGE bool operator<=(const Symbol &rhs) const override;

    ZENI_RETE_LINKAGE bool operator==(const Symbol_Constant_String &rhs) const override;
    ZENI_RETE_LINKAGE bool operator!=(const Symbol_Constant_String &rhs) const override;
    ZENI_RETE_LINKAGE bool operator<(const Symbol_Constant_String &rhs) const override;
    ZENI_RETE_LINKAGE bool operator<=(const Symbol_Constant_String &rhs) const override;
    ZENI_RETE_LINKAGE bool operator>(const Symbol_Constant_String &rhs) const override;
    ZENI_RETE_LINKAGE bool operator>=(const Symbol_Constant_String &rhs) const override;

    ZENI_RETE_LINKAGE bool operator>(const Symbol_Constant_Float &) const override;
    ZENI_RETE_LINKAGE bool operator>=(const Symbol_Constant_Float &) const override;
    ZENI_RETE_LINKAGE bool operator>(const Symbol_Constant_Int &) const override;
    ZENI_RETE_LINKAGE bool operator>=(const Symbol_Constant_Int &) const override;
    ZENI_RETE_LINKAGE bool operator<(const Symbol_Identifier &) const override;
    ZENI_RETE_LINKAGE bool operator<=(const Symbol_Identifier &) const override;
    ZENI_RETE_LINKAGE bool operator<(const Symbol_Variable &) const override;
    ZENI_RETE_LINKAGE bool operator<=(const Symbol_Variable &) const override;

    ZENI_RETE_LINKAGE bool operator==(const char * value_) const override;

    ZENI_RETE_LINKAGE size_t hash() const override;

    ZENI_RETE_LINKAGE virtual std::ostream & print(std::ostream &os) const override;

    ZENI_RETE_LINKAGE virtual std::ostream & print(std::ostream &os, const std::shared_ptr<const Variable_Indices>) const override;

  private:
    const std::string m_value;
  };

  class Symbol_Identifier : public Symbol {
    Symbol_Identifier(const Symbol_Identifier &) = delete;
    Symbol_Identifier & operator=(const Symbol_Identifier &) = delete;

  public:
    ZENI_RETE_LINKAGE Symbol_Identifier(const char * value_);

    ZENI_RETE_LINKAGE const char * get_value() const;

    ZENI_RETE_LINKAGE Symbol_Identifier * clone() const override;

    ZENI_RETE_LINKAGE bool operator==(const Symbol &rhs) const override;
    ZENI_RETE_LINKAGE bool operator!=(const Symbol &rhs) const override;
    ZENI_RETE_LINKAGE bool operator>(const Symbol &rhs) const override;
    ZENI_RETE_LINKAGE bool operator>=(const Symbol &rhs) const override;
    ZENI_RETE_LINKAGE bool operator<(const Symbol &rhs) const override;
    ZENI_RETE_LINKAGE bool operator<=(const Symbol &rhs) const override;

    ZENI_RETE_LINKAGE bool operator==(const Symbol_Identifier &rhs) const override;
    ZENI_RETE_LINKAGE bool operator!=(const Symbol_Identifier &rhs) const override;
    ZENI_RETE_LINKAGE bool operator<(const Symbol_Identifier &rhs) const override;
    ZENI_RETE_LINKAGE bool operator<=(const Symbol_Identifier &rhs) const override;
    ZENI_RETE_LINKAGE bool operator>(const Symbol_Identifier &rhs) const override;
    ZENI_RETE_LINKAGE bool operator>=(const Symbol_Identifier &rhs) const override;

    ZENI_RETE_LINKAGE bool operator>(const Symbol_Constant_Float &) const override;
    ZENI_RETE_LINKAGE bool operator>=(const Symbol_Constant_Float &) const override;
    ZENI_RETE_LINKAGE bool operator>(const Symbol_Constant_Int &) const override;
    ZENI_RETE_LINKAGE bool operator>=(const Symbol_Constant_Int &) const override;
    ZENI_RETE_LINKAGE bool operator>(const Symbol_Constant_String &) const override;
    ZENI_RETE_LINKAGE bool operator>=(const Symbol_Constant_String &) const override;
    ZENI_RETE_LINKAGE bool operator<(const Symbol_Variable &) const override;
    ZENI_RETE_LINKAGE bool operator<=(const Symbol_Variable &) const override;

    ZENI_RETE_LINKAGE bool operator==(const char * value_) const override;

    ZENI_RETE_LINKAGE size_t hash() const override;

    ZENI_RETE_LINKAGE virtual std::ostream & print(std::ostream &os) const override;

    ZENI_RETE_LINKAGE virtual std::ostream & print(std::ostream &os, const std::shared_ptr<const Variable_Indices>) const override;

  private:
    const std::string m_value;
  };

  class Symbol_Variable : public Symbol {
    Symbol_Variable(const Symbol_Variable &) = delete;
    Symbol_Variable & operator=(const Symbol_Variable &) = delete;

  public:
    enum Variable { First, Second, Third };

    ZENI_RETE_LINKAGE Symbol_Variable(const Variable &value_);

    ZENI_RETE_LINKAGE Symbol_Variable * clone() const override;

    ZENI_RETE_LINKAGE bool operator==(const Symbol &rhs) const override;
    ZENI_RETE_LINKAGE bool operator!=(const Symbol &rhs) const override;
    ZENI_RETE_LINKAGE bool operator>(const Symbol &rhs) const override;
    ZENI_RETE_LINKAGE bool operator>=(const Symbol &rhs) const override;
    ZENI_RETE_LINKAGE bool operator<(const Symbol &rhs) const override;
    ZENI_RETE_LINKAGE bool operator<=(const Symbol &rhs) const override;

    ZENI_RETE_LINKAGE bool operator==(const Symbol_Variable &rhs) const override;
    ZENI_RETE_LINKAGE bool operator!=(const Symbol_Variable &rhs) const override;
    ZENI_RETE_LINKAGE bool operator<(const Symbol_Variable &rhs) const override;
    ZENI_RETE_LINKAGE bool operator<=(const Symbol_Variable &rhs) const override;
    ZENI_RETE_LINKAGE bool operator>(const Symbol_Variable &rhs) const override;
    ZENI_RETE_LINKAGE bool operator>=(const Symbol_Variable &rhs) const override;

    ZENI_RETE_LINKAGE bool operator>(const Symbol_Constant_Float &) const override;
    ZENI_RETE_LINKAGE bool operator>=(const Symbol_Constant_Float &) const override;
    ZENI_RETE_LINKAGE bool operator>(const Symbol_Constant_Int &) const override;
    ZENI_RETE_LINKAGE bool operator>=(const Symbol_Constant_Int &) const override;
    ZENI_RETE_LINKAGE bool operator>(const Symbol_Constant_String &) const override;
    ZENI_RETE_LINKAGE bool operator>=(const Symbol_Constant_String &) const override;
    ZENI_RETE_LINKAGE bool operator>(const Symbol_Identifier &) const override;
    ZENI_RETE_LINKAGE bool operator>=(const Symbol_Identifier &) const override;

    ZENI_RETE_LINKAGE size_t hash() const override;

    ZENI_RETE_LINKAGE virtual std::ostream & print(std::ostream &os) const override;

    ZENI_RETE_LINKAGE virtual std::ostream & print(std::ostream &os, const std::shared_ptr<const Variable_Indices> indices) const override;

    const Variable value;
  };

}

bool ZENI_RETE_LINKAGE operator==(const double lhs, const Zeni::Rete::Symbol &rhs);
bool ZENI_RETE_LINKAGE operator==(const int64_t lhs, const Zeni::Rete::Symbol &rhs);
bool ZENI_RETE_LINKAGE operator==(const char * lhs, const Zeni::Rete::Symbol &rhs);

ZENI_RETE_LINKAGE std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Symbol &symbol);

namespace std {
  template <> struct hash<Zeni::Rete::Symbol> {
    size_t operator()(const Zeni::Rete::Symbol &symbol) const {
      return symbol.hash();
    }
  };
  template <> struct hash<Zeni::Rete::Symbol_Variable::Variable> {
    size_t operator()(const Zeni::Rete::Symbol_Variable::Variable &variable) const {
      return std::hash<size_t>()(variable);
    }
  };
}

#endif
