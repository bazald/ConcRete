#ifndef ZENI_RETE_SYMBOL_H
#define ZENI_RETE_SYMBOL_H

#include "Variable_Indices.h"

#include <memory>

namespace Zeni {

  namespace Rete {

    class Symbol;
    class Symbol_Constant;
    class Symbol_Constant_Float;
    class Symbol_Constant_Int;
    class Symbol_Constant_String;
    class Symbol_Identifier;
    class Symbol_Variable;

    class ZENI_RETE_LINKAGE Symbol
    {
      Symbol(const Symbol &) = delete;
      Symbol & operator=(const Symbol &) = delete;

    public:
      Symbol();
      virtual ~Symbol();

      virtual Symbol * clone() const = 0;

      virtual bool operator==(const Symbol &rhs) const = 0;
      virtual bool operator!=(const Symbol &rhs) const = 0;
      virtual bool operator<(const Symbol &rhs) const = 0;
      virtual bool operator<=(const Symbol &rhs) const = 0;
      virtual bool operator>(const Symbol &rhs) const = 0;
      virtual bool operator>=(const Symbol &rhs) const = 0;

      virtual bool operator==(const Symbol_Constant_Float &) const;
      virtual bool operator!=(const Symbol_Constant_Float &) const;
      virtual bool operator<(const Symbol_Constant_Float &) const;
      virtual bool operator<=(const Symbol_Constant_Float &) const;
      virtual bool operator>(const Symbol_Constant_Float &) const;
      virtual bool operator>=(const Symbol_Constant_Float &) const;

      virtual bool operator==(const Symbol_Constant_Int &) const;
      virtual bool operator!=(const Symbol_Constant_Int &) const;
      virtual bool operator<(const Symbol_Constant_Int &) const;
      virtual bool operator<=(const Symbol_Constant_Int &) const;
      virtual bool operator>(const Symbol_Constant_Int &) const;
      virtual bool operator>=(const Symbol_Constant_Int &) const;

      virtual bool operator==(const Symbol_Constant_String &) const;
      virtual bool operator!=(const Symbol_Constant_String &) const;
      virtual bool operator<(const Symbol_Constant_String &) const;
      virtual bool operator<=(const Symbol_Constant_String &) const;
      virtual bool operator>(const Symbol_Constant_String &) const;
      virtual bool operator>=(const Symbol_Constant_String &) const;

      virtual bool operator==(const Symbol_Identifier &) const;
      virtual bool operator!=(const Symbol_Identifier &) const;
      virtual bool operator<(const Symbol_Identifier &) const;
      virtual bool operator<=(const Symbol_Identifier &) const;
      virtual bool operator>(const Symbol_Identifier &) const;
      virtual bool operator>=(const Symbol_Identifier &) const;

      virtual bool operator==(const Symbol_Variable &) const;
      virtual bool operator!=(const Symbol_Variable &) const;
      virtual bool operator<(const Symbol_Variable &) const;
      virtual bool operator<=(const Symbol_Variable &) const;
      virtual bool operator>(const Symbol_Variable &) const;
      virtual bool operator>=(const Symbol_Variable &) const;

      virtual bool operator==(const double &) const;
      virtual bool operator==(const int64_t &) const;
      virtual bool operator==(const char * const &) const;

      virtual size_t hash() const = 0;
      virtual std::ostream & print(std::ostream &os) const = 0;
      virtual std::ostream & print(std::ostream &os, const std::shared_ptr<const Variable_Indices> &indices) const = 0;
    };

    class ZENI_RETE_LINKAGE Symbol_Constant : public Symbol {
      Symbol_Constant(const Symbol_Constant &) = delete;
      Symbol_Constant & operator=(const Symbol_Constant &) = delete;

    public:
      Symbol_Constant();
    };

    class ZENI_RETE_LINKAGE Symbol_Constant_Float : public Symbol_Constant {
      Symbol_Constant_Float(const Symbol_Constant_Float &) = delete;
      Symbol_Constant_Float & operator=(const Symbol_Constant_Float &) = delete;

    public:
      Symbol_Constant_Float(const double &value_);

      Symbol_Constant_Float * clone() const override;

      bool operator==(const Symbol &rhs) const override;
      bool operator!=(const Symbol &rhs) const override;
      bool operator>(const Symbol &rhs) const override;
      bool operator>=(const Symbol &rhs) const override;
      bool operator<(const Symbol &rhs) const override;
      bool operator<=(const Symbol &rhs) const override;

      bool operator==(const Symbol_Constant_Float &rhs) const override;
      bool operator!=(const Symbol_Constant_Float &rhs) const override;
      bool operator<(const Symbol_Constant_Float &rhs) const override;
      bool operator<=(const Symbol_Constant_Float &rhs) const override;
      bool operator>(const Symbol_Constant_Float &rhs) const override;
      bool operator>=(const Symbol_Constant_Float &rhs) const override;

      bool operator==(const Symbol_Constant_Int &rhs) const override;
      bool operator!=(const Symbol_Constant_Int &rhs) const override;
      bool operator<(const Symbol_Constant_Int &rhs) const override;
      bool operator<=(const Symbol_Constant_Int &rhs) const override;
      bool operator>(const Symbol_Constant_Int &rhs) const override;
      bool operator>=(const Symbol_Constant_Int &rhs) const override;

      bool operator<(const Symbol_Constant_String &) const override;
      bool operator<=(const Symbol_Constant_String &) const override;
      bool operator<(const Symbol_Identifier &) const override;
      bool operator<=(const Symbol_Identifier &) const override;
      bool operator<(const Symbol_Variable &) const override;
      bool operator<=(const Symbol_Variable &) const override;

      bool operator==(const double &value_) const override;

      size_t hash() const override;

      virtual std::ostream & print(std::ostream &os) const override;
      
      virtual std::ostream & print(std::ostream &os, const std::shared_ptr<const Variable_Indices> &) const override;

      const double value;
    };

    class ZENI_RETE_LINKAGE Symbol_Constant_Int : public Symbol_Constant {
      Symbol_Constant_Int(const Symbol_Constant_Int &) = delete;
      Symbol_Constant_Int & operator=(const Symbol_Constant_Int &) = delete;

    public:
      Symbol_Constant_Int(const int64_t &value_);

      Symbol_Constant_Int * clone() const override;

      bool operator==(const Symbol &rhs) const override;
      bool operator!=(const Symbol &rhs) const override;
      bool operator>(const Symbol &rhs) const override;
      bool operator>=(const Symbol &rhs) const override;
      bool operator<(const Symbol &rhs) const override;
      bool operator<=(const Symbol &rhs) const override;

      bool operator==(const Symbol_Constant_Float &rhs) const override;
      bool operator!=(const Symbol_Constant_Float &rhs) const override;
      bool operator<(const Symbol_Constant_Float &rhs) const override;
      bool operator<=(const Symbol_Constant_Float &rhs) const override;
      bool operator>(const Symbol_Constant_Float &rhs) const override;
      bool operator>=(const Symbol_Constant_Float &rhs) const override;

      bool operator==(const Symbol_Constant_Int &rhs) const override;
      bool operator!=(const Symbol_Constant_Int &rhs) const override;
      bool operator<(const Symbol_Constant_Int &rhs) const override;
      bool operator<=(const Symbol_Constant_Int &rhs) const override;
      bool operator>(const Symbol_Constant_Int &rhs) const override;
      bool operator>=(const Symbol_Constant_Int &rhs) const override;

      bool operator<(const Symbol_Constant_String &) const override;
      bool operator<=(const Symbol_Constant_String &) const override;
      bool operator<(const Symbol_Identifier &) const override;
      bool operator<=(const Symbol_Identifier &) const override;
      bool operator<(const Symbol_Variable &) const override;
      bool operator<=(const Symbol_Variable &) const override;

      bool operator==(const int64_t &value_) const override;

      size_t hash() const override;

      virtual std::ostream & print(std::ostream &os) const override;

      virtual std::ostream & print(std::ostream &os, const std::shared_ptr<const Variable_Indices> &) const override;

      const int64_t value;
    };

    class ZENI_RETE_LINKAGE Symbol_Constant_String : public Symbol_Constant {
      Symbol_Constant_String(const Symbol_Constant_String &) = delete;
      Symbol_Constant_String & operator=(const Symbol_Constant_String &) = delete;

    public:
      Symbol_Constant_String(const char * const &value_);

      const char * get_value() const;

      Symbol_Constant_String * clone() const override;

      bool operator==(const Symbol &rhs) const override;
      bool operator!=(const Symbol &rhs) const override;
      bool operator>(const Symbol &rhs) const override;
      bool operator>=(const Symbol &rhs) const override;
      bool operator<(const Symbol &rhs) const override;
      bool operator<=(const Symbol &rhs) const override;

      bool operator==(const Symbol_Constant_String &rhs) const override;
      bool operator!=(const Symbol_Constant_String &rhs) const override;
      bool operator<(const Symbol_Constant_String &rhs) const override;
      bool operator<=(const Symbol_Constant_String &rhs) const override;
      bool operator>(const Symbol_Constant_String &rhs) const override;
      bool operator>=(const Symbol_Constant_String &rhs) const override;

      bool operator>(const Symbol_Constant_Float &) const override;
      bool operator>=(const Symbol_Constant_Float &) const override;
      bool operator>(const Symbol_Constant_Int &) const override;
      bool operator>=(const Symbol_Constant_Int &) const override;
      bool operator<(const Symbol_Identifier &) const override;
      bool operator<=(const Symbol_Identifier &) const override;
      bool operator<(const Symbol_Variable &) const override;
      bool operator<=(const Symbol_Variable &) const override;

      bool operator==(const char * const &value_) const override;

      size_t hash() const override;

      virtual std::ostream & print(std::ostream &os) const override;

      virtual std::ostream & print(std::ostream &os, const std::shared_ptr<const Variable_Indices> &) const override;

    private:
#pragma warning( push )
#pragma warning( disable: 4251 )
      const std::string m_value;
#pragma warning( pop )
    };

    class ZENI_RETE_LINKAGE Symbol_Identifier : public Symbol {
      Symbol_Identifier(const Symbol_Identifier &) = delete;
      Symbol_Identifier & operator=(const Symbol_Identifier &) = delete;

    public:
      Symbol_Identifier(const char * const &value_);

      const char * get_value() const;

      Symbol_Identifier * clone() const override;

      bool operator==(const Symbol &rhs) const override;
      bool operator!=(const Symbol &rhs) const override;
      bool operator>(const Symbol &rhs) const override;
      bool operator>=(const Symbol &rhs) const override;
      bool operator<(const Symbol &rhs) const override;
      bool operator<=(const Symbol &rhs) const override;

      bool operator==(const Symbol_Identifier &rhs) const override;
      bool operator!=(const Symbol_Identifier &rhs) const override;
      bool operator<(const Symbol_Identifier &rhs) const override;
      bool operator<=(const Symbol_Identifier &rhs) const override;
      bool operator>(const Symbol_Identifier &rhs) const override;
      bool operator>=(const Symbol_Identifier &rhs) const override;

      bool operator>(const Symbol_Constant_Float &) const override;
      bool operator>=(const Symbol_Constant_Float &) const override;
      bool operator>(const Symbol_Constant_Int &) const override;
      bool operator>=(const Symbol_Constant_Int &) const override;
      bool operator>(const Symbol_Constant_String &) const override;
      bool operator>=(const Symbol_Constant_String &) const override;
      bool operator<(const Symbol_Variable &) const override;
      bool operator<=(const Symbol_Variable &) const override;

      bool operator==(const char * const &value_) const override;

      size_t hash() const override;

      virtual std::ostream & print(std::ostream &os) const override;

      virtual std::ostream & print(std::ostream &os, const std::shared_ptr<const Variable_Indices> &) const override;

    private:
#pragma warning( push )
#pragma warning( disable: 4251 )
      const std::string m_value;
#pragma warning( pop )
    };

    class ZENI_RETE_LINKAGE Symbol_Variable : public Symbol {
      Symbol_Variable(const Symbol_Variable &) = delete;
      Symbol_Variable & operator=(const Symbol_Variable &) = delete;

    public:
      enum Variable { First, Second, Third };

      Symbol_Variable(const Variable &value_);

      Symbol_Variable * clone() const override;

      bool operator==(const Symbol &rhs) const override;
      bool operator!=(const Symbol &rhs) const override;
      bool operator>(const Symbol &rhs) const override;
      bool operator>=(const Symbol &rhs) const override;
      bool operator<(const Symbol &rhs) const override;
      bool operator<=(const Symbol &rhs) const override;

      bool operator==(const Symbol_Variable &rhs) const override;
      bool operator!=(const Symbol_Variable &rhs) const override;
      bool operator<(const Symbol_Variable &rhs) const override;
      bool operator<=(const Symbol_Variable &rhs) const override;
      bool operator>(const Symbol_Variable &rhs) const override;
      bool operator>=(const Symbol_Variable &rhs) const override;

      bool operator>(const Symbol_Constant_Float &) const override;
      bool operator>=(const Symbol_Constant_Float &) const override;
      bool operator>(const Symbol_Constant_Int &) const override;
      bool operator>=(const Symbol_Constant_Int &) const override;
      bool operator>(const Symbol_Constant_String &) const override;
      bool operator>=(const Symbol_Constant_String &) const override;
      bool operator>(const Symbol_Identifier &) const override;
      bool operator>=(const Symbol_Identifier &) const override;

      size_t hash() const override;

      virtual std::ostream & print(std::ostream &os) const override;

      virtual std::ostream & print(std::ostream &os, const std::shared_ptr<const Variable_Indices> &indices) const override;

      const Variable value;
    };

  }

}

bool ZENI_RETE_LINKAGE operator==(const double &lhs, const Zeni::Rete::Symbol &rhs);
bool ZENI_RETE_LINKAGE operator==(const int64_t &lhs, const Zeni::Rete::Symbol &rhs);
bool ZENI_RETE_LINKAGE operator==(const char * const &lhs, const Zeni::Rete::Symbol &rhs);

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
