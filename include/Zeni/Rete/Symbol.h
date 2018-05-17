#ifndef ZENI_RETE_SYMBOL_H
#define ZENI_RETE_SYMBOL_H

#include "Variable_Indices.h"

#include <array>
#include <map>
#include <memory>
#include <typeindex>

namespace Zeni {

  namespace Rete {

    class Symbol;
    class Symbol_Constant;
    class Symbol_Constant_Float;
    class Symbol_Constant_Int;
    class Symbol_Constant_String;
    class Symbol_Identifier;
    class Symbol_Variable;

    typedef std::shared_ptr<const Symbol> Symbol_Ptr_C;
    typedef std::shared_ptr<const Symbol_Constant> Symbol_Constant_Ptr_C;
    typedef std::shared_ptr<const Symbol_Constant_Float> Symbol_Constant_Float_Ptr_C;
    typedef std::shared_ptr<const Symbol_Constant_Int> Symbol_Constant_Int_Ptr_C;
    typedef std::shared_ptr<const Symbol_Constant_String> Symbol_Constant_String_Ptr_C;
    typedef std::shared_ptr<const Symbol_Identifier> Symbol_Identifier_Ptr_C;
    typedef std::shared_ptr<const Symbol_Variable> Symbol_Variable_Ptr_C;
    typedef std::shared_ptr<const Variable_Indices> Variable_Indices_Ptr_C;

    class ZENI_RETE_LINKAGE Symbol
    {
      Symbol(const Symbol &);
      Symbol & operator=(const Symbol &);

    public:
      Symbol() {}
      virtual ~Symbol() {}

      virtual Symbol * clone() const = 0;

      virtual bool operator==(const Symbol &rhs) const = 0;
      virtual bool operator!=(const Symbol &rhs) const = 0;
      virtual bool operator<(const Symbol &rhs) const = 0;
      virtual bool operator<=(const Symbol &rhs) const = 0;
      virtual bool operator>(const Symbol &rhs) const = 0;
      virtual bool operator>=(const Symbol &rhs) const = 0;

      virtual bool operator==(const Symbol_Constant_Float &) const { return false; }
      virtual bool operator!=(const Symbol_Constant_Float &) const { return true; }
      virtual bool operator<(const Symbol_Constant_Float &) const { return false; }
      virtual bool operator<=(const Symbol_Constant_Float &) const { return false; }
      virtual bool operator>(const Symbol_Constant_Float &) const { return false; }
      virtual bool operator>=(const Symbol_Constant_Float &) const { return false; }

      virtual bool operator==(const Symbol_Constant_Int &) const { return false; }
      virtual bool operator!=(const Symbol_Constant_Int &) const { return true; }
      virtual bool operator<(const Symbol_Constant_Int &) const { return false; }
      virtual bool operator<=(const Symbol_Constant_Int &) const { return false; }
      virtual bool operator>(const Symbol_Constant_Int &) const { return false; }
      virtual bool operator>=(const Symbol_Constant_Int &) const { return false; }

      virtual bool operator==(const Symbol_Constant_String &) const { return false; }
      virtual bool operator!=(const Symbol_Constant_String &) const { return true; }
      virtual bool operator<(const Symbol_Constant_String &) const { return false; }
      virtual bool operator<=(const Symbol_Constant_String &) const { return false; }
      virtual bool operator>(const Symbol_Constant_String &) const { return false; }
      virtual bool operator>=(const Symbol_Constant_String &) const { return false; }

      virtual bool operator==(const Symbol_Identifier &) const { return false; }
      virtual bool operator!=(const Symbol_Identifier &) const { return true; }
      virtual bool operator<(const Symbol_Identifier &) const { return false; }
      virtual bool operator<=(const Symbol_Identifier &) const { return false; }
      virtual bool operator>(const Symbol_Identifier &) const { return false; }
      virtual bool operator>=(const Symbol_Identifier &) const { return false; }

      virtual bool operator==(const Symbol_Variable &) const { return false; }
      virtual bool operator!=(const Symbol_Variable &) const { return true; }
      virtual bool operator<(const Symbol_Variable &) const { return false; }
      virtual bool operator<=(const Symbol_Variable &) const { return false; }
      virtual bool operator>(const Symbol_Variable &) const { return false; }
      virtual bool operator>=(const Symbol_Variable &) const { return false; }

      virtual bool operator==(const double &) const { return false; }
      virtual bool operator==(const int64_t &) const { return false; }
      virtual bool operator==(const std::string &) const { return false; }

      virtual size_t hash() const = 0;
      virtual std::ostream & print(std::ostream &os) const = 0;
      virtual std::ostream & print(std::ostream &os, const Variable_Indices_Ptr_C &indices) const = 0;
    };

    class ZENI_RETE_LINKAGE Symbol_Constant : public Symbol {
      Symbol_Constant(const Symbol_Constant &);
      Symbol_Constant & operator=(const Symbol_Constant &);

    public:
      Symbol_Constant() {}
    };

    class ZENI_RETE_LINKAGE Symbol_Constant_Float : public Symbol_Constant {
      Symbol_Constant_Float(const Symbol_Constant_Float &);
      Symbol_Constant_Float & operator=(const Symbol_Constant_Float &);

    public:
      Symbol_Constant_Float(const double &value_) : value(value_) {}

      Symbol_Constant_Float * clone() const override { return new Symbol_Constant_Float(value); }

      bool operator==(const Symbol &rhs) const override { return rhs == *this; }
      bool operator!=(const Symbol &rhs) const override { return rhs != *this; }
      bool operator>(const Symbol &rhs) const override { return rhs < *this; }
      bool operator>=(const Symbol &rhs) const override { return rhs <= *this; }
      bool operator<(const Symbol &rhs) const override { return rhs > *this; }
      bool operator<=(const Symbol &rhs) const override { return rhs >= *this; }

      bool operator==(const Symbol_Constant_Float &rhs) const override { return value == rhs.value; }
      bool operator!=(const Symbol_Constant_Float &rhs) const override { return value != rhs.value; }
      bool operator<(const Symbol_Constant_Float &rhs) const override { return value < rhs.value; }
      bool operator<=(const Symbol_Constant_Float &rhs) const override { return value <= rhs.value; }
      bool operator>(const Symbol_Constant_Float &rhs) const override { return value > rhs.value; }
      bool operator>=(const Symbol_Constant_Float &rhs) const override { return value >= rhs.value; }

      inline bool operator==(const Symbol_Constant_Int &rhs) const override;
      inline bool operator!=(const Symbol_Constant_Int &rhs) const override;
      inline bool operator<(const Symbol_Constant_Int &rhs) const override;
      inline bool operator<=(const Symbol_Constant_Int &rhs) const override;
      inline bool operator>(const Symbol_Constant_Int &rhs) const override;
      inline bool operator>=(const Symbol_Constant_Int &rhs) const override;

      bool operator<(const Symbol_Constant_String &) const override { return true; }
      bool operator<=(const Symbol_Constant_String &) const override { return true; }
      bool operator<(const Symbol_Identifier &) const override { return true; }
      bool operator<=(const Symbol_Identifier &) const override { return true; }
      bool operator<(const Symbol_Variable &) const override { return true; }
      bool operator<=(const Symbol_Variable &) const override { return true; }

      bool operator==(const double &value_) const override { return value_ == value; }

      size_t hash() const override {
        return std::hash<double>()(value);
      }

      virtual std::ostream & print(std::ostream &os) const override {
        return os << Rete::to_string(value);
      }

      virtual std::ostream & print(std::ostream &os, const Variable_Indices_Ptr_C &) const override {
        return os << Rete::to_string(value);
      }

      const double value;
    };

    class ZENI_RETE_LINKAGE Symbol_Constant_Int : public Symbol_Constant {
      Symbol_Constant_Int(const Symbol_Constant_Int &);
      Symbol_Constant_Int & operator=(const Symbol_Constant_Int &);

    public:
      Symbol_Constant_Int(const int64_t &value_) : value(value_) {}

      Symbol_Constant_Int * clone() const override { return new Symbol_Constant_Int(value); }

      bool operator==(const Symbol &rhs) const override { return rhs == *this; }
      bool operator!=(const Symbol &rhs) const override { return rhs != *this; }
      bool operator>(const Symbol &rhs) const override { return rhs < *this; }
      bool operator>=(const Symbol &rhs) const override { return rhs <= *this; }
      bool operator<(const Symbol &rhs) const override { return rhs > *this; }
      bool operator<=(const Symbol &rhs) const override { return rhs >= *this; }

      bool operator==(const Symbol_Constant_Float &rhs) const override { return value == rhs.value; }
      bool operator!=(const Symbol_Constant_Float &rhs) const override { return value != rhs.value; }
      bool operator<(const Symbol_Constant_Float &rhs) const override { return value < rhs.value; }
      bool operator<=(const Symbol_Constant_Float &rhs) const override { return value <= rhs.value; }
      bool operator>(const Symbol_Constant_Float &rhs) const override { return value > rhs.value; }
      bool operator>=(const Symbol_Constant_Float &rhs) const override { return value >= rhs.value; }

      bool operator==(const Symbol_Constant_Int &rhs) const override { return value == rhs.value; }
      bool operator!=(const Symbol_Constant_Int &rhs) const override { return value != rhs.value; }
      bool operator<(const Symbol_Constant_Int &rhs) const override { return value < rhs.value; }
      bool operator<=(const Symbol_Constant_Int &rhs) const override { return value <= rhs.value; }
      bool operator>(const Symbol_Constant_Int &rhs) const override { return value > rhs.value; }
      bool operator>=(const Symbol_Constant_Int &rhs) const override { return value >= rhs.value; }

      bool operator<(const Symbol_Constant_String &) const override { return true; }
      bool operator<=(const Symbol_Constant_String &) const override { return true; }
      bool operator<(const Symbol_Identifier &) const override { return true; }
      bool operator<=(const Symbol_Identifier &) const override { return true; }
      bool operator<(const Symbol_Variable &) const override { return true; }
      bool operator<=(const Symbol_Variable &) const override { return true; }

      bool operator==(const int64_t &value_) const override { return value_ == value; }

      size_t hash() const override {
        return std::hash<int64_t>()(value);
      }

      virtual std::ostream & print(std::ostream &os) const override {
        return os << value;
      }

      virtual std::ostream & print(std::ostream &os, const Variable_Indices_Ptr_C &) const override {
        return os << value;
      }

      const int64_t value;
    };

    bool Symbol_Constant_Float::operator==(const Symbol_Constant_Int &rhs) const { return value == rhs.value; }
    bool Symbol_Constant_Float::operator!=(const Symbol_Constant_Int &rhs) const { return value != rhs.value; }
    bool Symbol_Constant_Float::operator<(const Symbol_Constant_Int &rhs) const { return value < rhs.value; }
    bool Symbol_Constant_Float::operator<=(const Symbol_Constant_Int &rhs) const { return value <= rhs.value; }
    bool Symbol_Constant_Float::operator>(const Symbol_Constant_Int &rhs) const { return value > rhs.value; }
    bool Symbol_Constant_Float::operator>=(const Symbol_Constant_Int &rhs) const { return value >= rhs.value; }

    class ZENI_RETE_LINKAGE Symbol_Constant_String : public Symbol_Constant {
      Symbol_Constant_String(const Symbol_Constant_String &);
      Symbol_Constant_String & operator=(const Symbol_Constant_String &);

    public:
      Symbol_Constant_String(const std::string &value_) : value(value_) {}

      Symbol_Constant_String * clone() const override { return new Symbol_Constant_String(value); }

      bool operator==(const Symbol &rhs) const override { return rhs == *this; }
      bool operator!=(const Symbol &rhs) const override { return rhs != *this; }
      bool operator>(const Symbol &rhs) const override { return rhs < *this; }
      bool operator>=(const Symbol &rhs) const override { return rhs <= *this; }
      bool operator<(const Symbol &rhs) const override { return rhs > *this; }
      bool operator<=(const Symbol &rhs) const override { return rhs >= *this; }

      bool operator==(const Symbol_Constant_String &rhs) const override { return value == rhs.value; }
      bool operator!=(const Symbol_Constant_String &rhs) const override { return value != rhs.value; }
      bool operator<(const Symbol_Constant_String &rhs) const override { return value < rhs.value; }
      bool operator<=(const Symbol_Constant_String &rhs) const override { return value <= rhs.value; }
      bool operator>(const Symbol_Constant_String &rhs) const override { return value > rhs.value; }
      bool operator>=(const Symbol_Constant_String &rhs) const override { return value >= rhs.value; }

      bool operator>(const Symbol_Constant_Float &) const override { return true; }
      bool operator>=(const Symbol_Constant_Float &) const override { return true; }
      bool operator>(const Symbol_Constant_Int &) const override { return true; }
      bool operator>=(const Symbol_Constant_Int &) const override { return true; }
      bool operator<(const Symbol_Identifier &) const override { return true; }
      bool operator<=(const Symbol_Identifier &) const override { return true; }
      bool operator<(const Symbol_Variable &) const override { return true; }
      bool operator<=(const Symbol_Variable &) const override { return true; }

      bool operator==(const std::string &value_) const override { return value_ == value; }

      size_t hash() const override {
        return std::hash<std::string>()(value);
      }

      virtual std::ostream & print(std::ostream &os) const override {
        if (value.find_first_of(" \t\r\n") == std::string::npos)
          return os << value;
        else
          return os << '|' << value << '|';
      }

      virtual std::ostream & print(std::ostream &os, const Variable_Indices_Ptr_C &) const override {
        return print(os);
      }

      const std::string value;
    };

    class ZENI_RETE_LINKAGE Symbol_Identifier : public Symbol {
      Symbol_Identifier(const Symbol_Identifier &);
      Symbol_Identifier & operator=(const Symbol_Identifier &);

    public:
      Symbol_Identifier(const std::string &value_) : value(value_) {}

      Symbol_Identifier * clone() const override { return new Symbol_Identifier(value); }

      bool operator==(const Symbol &rhs) const override { return rhs == *this; }
      bool operator!=(const Symbol &rhs) const override { return rhs != *this; }
      bool operator>(const Symbol &rhs) const override { return rhs < *this; }
      bool operator>=(const Symbol &rhs) const override { return rhs <= *this; }
      bool operator<(const Symbol &rhs) const override { return rhs > *this; }
      bool operator<=(const Symbol &rhs) const override { return rhs >= *this; }

      bool operator==(const Symbol_Identifier &rhs) const override { return value == rhs.value; }
      bool operator!=(const Symbol_Identifier &rhs) const override { return value != rhs.value; }
      bool operator<(const Symbol_Identifier &rhs) const override { return value < rhs.value; }
      bool operator<=(const Symbol_Identifier &rhs) const override { return value <= rhs.value; }
      bool operator>(const Symbol_Identifier &rhs) const override { return value > rhs.value; }
      bool operator>=(const Symbol_Identifier &rhs) const override { return value >= rhs.value; }

      bool operator>(const Symbol_Constant_Float &) const override { return true; }
      bool operator>=(const Symbol_Constant_Float &) const override { return true; }
      bool operator>(const Symbol_Constant_Int &) const override { return true; }
      bool operator>=(const Symbol_Constant_Int &) const override { return true; }
      bool operator>(const Symbol_Constant_String &) const override { return true; }
      bool operator>=(const Symbol_Constant_String &) const override { return true; }
      bool operator<(const Symbol_Variable &) const override { return true; }
      bool operator<=(const Symbol_Variable &) const override { return true; }

      bool operator==(const std::string &value_) const override { return value_ == value; }

      size_t hash() const override {
        return std::hash<std::string>()(value);
      }

      virtual std::ostream & print(std::ostream &os) const override {
        return os << value;
      }

      virtual std::ostream & print(std::ostream &os, const Variable_Indices_Ptr_C &) const override {
        return os << value;
      }

      const std::string value;
    };

    class ZENI_RETE_LINKAGE Symbol_Variable : public Symbol {
      Symbol_Variable(const Symbol_Variable &);
      Symbol_Variable & operator=(const Symbol_Variable &);

    public:
      enum Variable { First, Second, Third };

      Symbol_Variable(const Variable &value_) : value(value_) {}

      Symbol_Variable * clone() const override { return new Symbol_Variable(value); }

      bool operator==(const Symbol &rhs) const override { return rhs == *this; }
      bool operator!=(const Symbol &rhs) const override { return rhs != *this; }
      bool operator>(const Symbol &rhs) const override { return rhs < *this; }
      bool operator>=(const Symbol &rhs) const override { return rhs <= *this; }
      bool operator<(const Symbol &rhs) const override { return rhs > *this; }
      bool operator<=(const Symbol &rhs) const override { return rhs >= *this; }

      bool operator==(const Symbol_Variable &rhs) const override { return value == rhs.value; }
      bool operator!=(const Symbol_Variable &rhs) const override { return value != rhs.value; }
      bool operator<(const Symbol_Variable &rhs) const override { return value < rhs.value; }
      bool operator<=(const Symbol_Variable &rhs) const override { return value <= rhs.value; }
      bool operator>(const Symbol_Variable &rhs) const override { return value > rhs.value; }
      bool operator>=(const Symbol_Variable &rhs) const override { return value >= rhs.value; }

      bool operator>(const Symbol_Constant_Float &) const override { return true; }
      bool operator>=(const Symbol_Constant_Float &) const override { return true; }
      bool operator>(const Symbol_Constant_Int &) const override { return true; }
      bool operator>=(const Symbol_Constant_Int &) const override { return true; }
      bool operator>(const Symbol_Constant_String &) const override { return true; }
      bool operator>=(const Symbol_Constant_String &) const override { return true; }
      bool operator>(const Symbol_Identifier &) const override { return true; }
      bool operator>=(const Symbol_Identifier &) const override { return true; }

      size_t hash() const override {
        return std::hash<size_t>()(value);
      }

      virtual std::ostream & print(std::ostream &os) const override {
        os.put('<');
        os << value;
        os.put('>');
        return os;
      }

      virtual std::ostream & print(std::ostream &os, const Variable_Indices_Ptr_C &indices) const override {
        os.put('<');
        const auto found = std::find_if(indices->begin(), indices->end(), [this](const std::pair<std::string, WME_Token_Index> &ind)->bool {
          return ind.second.token_row == 0 && ind.second.column == this->value;
        });
        if (found != indices->end())
          os << found->first;
        else
          os << value;
        os.put('>');
        return os;
      }

      const Variable value;
    };

    inline void __symbol_size_check() {
      typedef Symbol::value_type pool_allocator_type;
      static_assert(sizeof(pool_allocator_type) >= sizeof(Symbol_Constant_Float), "Pool size suboptimal.");
      static_assert(sizeof(pool_allocator_type) >= sizeof(Symbol_Constant_Int), "Pool size suboptimal.");
      static_assert(sizeof(pool_allocator_type) >= sizeof(Symbol_Constant_String), "Pool size suboptimal.");
      static_assert(sizeof(pool_allocator_type) >= sizeof(Symbol_Identifier), "Pool size suboptimal.");
      static_assert(sizeof(pool_allocator_type) >= sizeof(Symbol_Variable), "Pool size suboptimal.");
    }

  }

}

inline bool operator==(const double &lhs, const Rete::Symbol &rhs) {return rhs == lhs;}
inline bool operator==(const int64_t &lhs, const Rete::Symbol &rhs) {return rhs == lhs;}
inline bool operator==(const std::string &lhs, const Rete::Symbol &rhs) {return rhs == lhs;}

inline std::ostream & operator<<(std::ostream &os, const Rete::Symbol &symbol) {
  return symbol.print(os);
}

inline std::ostream & operator<<(std::ostream &os, const Rete::Variable_Indices &indices) {
  os << '{';
  for(const auto &index : indices) {
    os << '[' << index.first << ',' << index.second << ']';
  }
  os << '}';
  return os;
}

namespace std {
  template <> struct hash<Rete::Symbol> {
    size_t operator()(const Rete::Symbol &symbol) const {
      return symbol.hash();
    }
  };
  template <> struct hash<Rete::Symbol_Variable::Variable> {
    size_t operator()(const Rete::Symbol_Variable::Variable &variable) const {
      return std::hash<size_t>()(variable);
    }
  };
}

#endif
