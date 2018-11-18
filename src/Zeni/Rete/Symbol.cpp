#include "Zeni/Rete/Symbol.hpp"

#include "Zeni/Rete/Variable_Indices.hpp"
#include "Zeni/Utility.hpp"
#include <algorithm>
#include <iostream>
#include <string_view>

namespace Zeni::Rete {

  Symbol::Symbol() {}
  Symbol::~Symbol() {}

  bool Symbol::operator==(const Symbol_Constant_Float &) const { return false; }
  bool Symbol::operator!=(const Symbol_Constant_Float &) const { return true; }
  bool Symbol::operator<(const Symbol_Constant_Float &) const { return false; }
  bool Symbol::operator<=(const Symbol_Constant_Float &) const { return false; }
  bool Symbol::operator>(const Symbol_Constant_Float &) const { return false; }
  bool Symbol::operator>=(const Symbol_Constant_Float &) const { return false; }

  bool Symbol::operator==(const Symbol_Constant_Int &) const { return false; }
  bool Symbol::operator!=(const Symbol_Constant_Int &) const { return true; }
  bool Symbol::operator<(const Symbol_Constant_Int &) const { return false; }
  bool Symbol::operator<=(const Symbol_Constant_Int &) const { return false; }
  bool Symbol::operator>(const Symbol_Constant_Int &) const { return false; }
  bool Symbol::operator>=(const Symbol_Constant_Int &) const { return false; }

  bool Symbol::operator==(const Symbol_Constant_String &) const { return false; }
  bool Symbol::operator!=(const Symbol_Constant_String &) const { return true; }
  bool Symbol::operator<(const Symbol_Constant_String &) const { return false; }
  bool Symbol::operator<=(const Symbol_Constant_String &) const { return false; }
  bool Symbol::operator>(const Symbol_Constant_String &) const { return false; }
  bool Symbol::operator>=(const Symbol_Constant_String &) const { return false; }

  bool Symbol::operator==(const Symbol_Constant_Identifier &) const { return false; }
  bool Symbol::operator!=(const Symbol_Constant_Identifier &) const { return true; }
  bool Symbol::operator<(const Symbol_Constant_Identifier &) const { return false; }
  bool Symbol::operator<=(const Symbol_Constant_Identifier &) const { return false; }
  bool Symbol::operator>(const Symbol_Constant_Identifier &) const { return false; }
  bool Symbol::operator>=(const Symbol_Constant_Identifier &) const { return false; }

  bool Symbol::operator==(const Symbol_Variable &) const { return false; }
  bool Symbol::operator!=(const Symbol_Variable &) const { return true; }
  bool Symbol::operator<(const Symbol_Variable &) const { return false; }
  bool Symbol::operator<=(const Symbol_Variable &) const { return false; }
  bool Symbol::operator>(const Symbol_Variable &) const { return false; }
  bool Symbol::operator>=(const Symbol_Variable &) const { return false; }

  bool Symbol::operator==(const double) const { return false; }
  bool Symbol::operator==(const int64_t) const { return false; }
  bool Symbol::operator==(const char *) const { return false; }


  Symbol_Constant::Symbol_Constant() {}


  Symbol_Constant_Float::Symbol_Constant_Float(const double &value_) : value(value_) {}

  Symbol_Constant_Float * Symbol_Constant_Float::clone() const { return new Symbol_Constant_Float(value); }

  bool Symbol_Constant_Float::operator==(const Symbol &rhs) const { return rhs == *this; }
  bool Symbol_Constant_Float::operator!=(const Symbol &rhs) const { return rhs != *this; }
  bool Symbol_Constant_Float::operator>(const Symbol &rhs) const { return rhs < *this; }
  bool Symbol_Constant_Float::operator>=(const Symbol &rhs) const { return rhs <= *this; }
  bool Symbol_Constant_Float::operator<(const Symbol &rhs) const { return rhs > *this; }
  bool Symbol_Constant_Float::operator<=(const Symbol &rhs) const { return rhs >= *this; }

  bool Symbol_Constant_Float::operator==(const Symbol_Constant_Float &rhs) const { return value == rhs.value; }
  bool Symbol_Constant_Float::operator!=(const Symbol_Constant_Float &rhs) const { return value != rhs.value; }
  bool Symbol_Constant_Float::operator<(const Symbol_Constant_Float &rhs) const { return value < rhs.value; }
  bool Symbol_Constant_Float::operator<=(const Symbol_Constant_Float &rhs) const { return value <= rhs.value; }
  bool Symbol_Constant_Float::operator>(const Symbol_Constant_Float &rhs) const { return value > rhs.value; }
  bool Symbol_Constant_Float::operator>=(const Symbol_Constant_Float &rhs) const { return value >= rhs.value; }

  bool Symbol_Constant_Float::operator==(const Symbol_Constant_Int &rhs) const { return value == rhs.value; }
  bool Symbol_Constant_Float::operator!=(const Symbol_Constant_Int &rhs) const { return value != rhs.value; }
  bool Symbol_Constant_Float::operator<(const Symbol_Constant_Int &rhs) const { return value < rhs.value; }
  bool Symbol_Constant_Float::operator<=(const Symbol_Constant_Int &rhs) const { return value <= rhs.value; }
  bool Symbol_Constant_Float::operator>(const Symbol_Constant_Int &rhs) const { return value > rhs.value; }
  bool Symbol_Constant_Float::operator>=(const Symbol_Constant_Int &rhs) const { return value >= rhs.value; }

  bool Symbol_Constant_Float::operator<(const Symbol_Constant_String &) const { return true; }
  bool Symbol_Constant_Float::operator<=(const Symbol_Constant_String &) const { return true; }
  bool Symbol_Constant_Float::operator<(const Symbol_Constant_Identifier &) const { return true; }
  bool Symbol_Constant_Float::operator<=(const Symbol_Constant_Identifier &) const { return true; }
  bool Symbol_Constant_Float::operator<(const Symbol_Variable &) const { return true; }
  bool Symbol_Constant_Float::operator<=(const Symbol_Variable &) const { return true; }

  bool Symbol_Constant_Float::operator==(const double value_) const { return value_ == value; }

  size_t Symbol_Constant_Float::hash() const {
    return std::hash<double>()(value);
  }

  std::ostream & Symbol_Constant_Float::print(std::ostream &os) const {
    return os << Zeni::to_string(value);
  }

  std::ostream & Symbol_Constant_Float::print_contents(std::ostream &os) const {
    return os << Zeni::to_string(value);
  }


  Symbol_Constant_Int::Symbol_Constant_Int(const int64_t &value_) : value(value_) {}

  Symbol_Constant_Int * Symbol_Constant_Int::clone() const { return new Symbol_Constant_Int(value); }

  bool Symbol_Constant_Int::operator==(const Symbol &rhs) const { return rhs == *this; }
  bool Symbol_Constant_Int::operator!=(const Symbol &rhs) const { return rhs != *this; }
  bool Symbol_Constant_Int::operator>(const Symbol &rhs) const { return rhs < *this; }
  bool Symbol_Constant_Int::operator>=(const Symbol &rhs) const { return rhs <= *this; }
  bool Symbol_Constant_Int::operator<(const Symbol &rhs) const { return rhs > *this; }
  bool Symbol_Constant_Int::operator<=(const Symbol &rhs) const { return rhs >= *this; }

  bool Symbol_Constant_Int::operator==(const Symbol_Constant_Float &rhs) const { return value == rhs.value; }
  bool Symbol_Constant_Int::operator!=(const Symbol_Constant_Float &rhs) const { return value != rhs.value; }
  bool Symbol_Constant_Int::operator<(const Symbol_Constant_Float &rhs) const { return value < rhs.value; }
  bool Symbol_Constant_Int::operator<=(const Symbol_Constant_Float &rhs) const { return value <= rhs.value; }
  bool Symbol_Constant_Int::operator>(const Symbol_Constant_Float &rhs) const { return value > rhs.value; }
  bool Symbol_Constant_Int::operator>=(const Symbol_Constant_Float &rhs) const { return value >= rhs.value; }

  bool Symbol_Constant_Int::operator==(const Symbol_Constant_Int &rhs) const { return value == rhs.value; }
  bool Symbol_Constant_Int::operator!=(const Symbol_Constant_Int &rhs) const { return value != rhs.value; }
  bool Symbol_Constant_Int::operator<(const Symbol_Constant_Int &rhs) const { return value < rhs.value; }
  bool Symbol_Constant_Int::operator<=(const Symbol_Constant_Int &rhs) const { return value <= rhs.value; }
  bool Symbol_Constant_Int::operator>(const Symbol_Constant_Int &rhs) const { return value > rhs.value; }
  bool Symbol_Constant_Int::operator>=(const Symbol_Constant_Int &rhs) const { return value >= rhs.value; }

  bool Symbol_Constant_Int::operator<(const Symbol_Constant_String &) const { return true; }
  bool Symbol_Constant_Int::operator<=(const Symbol_Constant_String &) const { return true; }
  bool Symbol_Constant_Int::operator<(const Symbol_Constant_Identifier &) const { return true; }
  bool Symbol_Constant_Int::operator<=(const Symbol_Constant_Identifier &) const { return true; }
  bool Symbol_Constant_Int::operator<(const Symbol_Variable &) const { return true; }
  bool Symbol_Constant_Int::operator<=(const Symbol_Variable &) const { return true; }

  bool Symbol_Constant_Int::operator==(const int64_t value_) const { return value_ == value; }

  size_t Symbol_Constant_Int::hash() const {
    return std::hash<int64_t>()(value);
  }

  std::ostream & Symbol_Constant_Int::print(std::ostream &os) const {
    return os << value;
  }

  std::ostream & Symbol_Constant_Int::print_contents(std::ostream &os) const {
    return os << value;
  }


  Symbol_Constant_String::Symbol_Constant_String(const std::string &value_) : m_value(value_) {}
  Symbol_Constant_String::Symbol_Constant_String(std::string &&value_) : m_value(std::move(value_)) {}

  std::string_view Symbol_Constant_String::get_value() const { return m_value; }

  Symbol_Constant_String * Symbol_Constant_String::clone() const { return new Symbol_Constant_String(m_value); }

  bool Symbol_Constant_String::operator==(const Symbol &rhs) const { return rhs == *this; }
  bool Symbol_Constant_String::operator!=(const Symbol &rhs) const { return rhs != *this; }
  bool Symbol_Constant_String::operator>(const Symbol &rhs) const { return rhs < *this; }
  bool Symbol_Constant_String::operator>=(const Symbol &rhs) const { return rhs <= *this; }
  bool Symbol_Constant_String::operator<(const Symbol &rhs) const { return rhs > *this; }
  bool Symbol_Constant_String::operator<=(const Symbol &rhs) const { return rhs >= *this; }

  bool Symbol_Constant_String::operator==(const Symbol_Constant_String &rhs) const { return m_value == rhs.m_value; }
  bool Symbol_Constant_String::operator!=(const Symbol_Constant_String &rhs) const { return m_value != rhs.m_value; }
  bool Symbol_Constant_String::operator<(const Symbol_Constant_String &rhs) const { return m_value < rhs.m_value; }
  bool Symbol_Constant_String::operator<=(const Symbol_Constant_String &rhs) const { return m_value <= rhs.m_value; }
  bool Symbol_Constant_String::operator>(const Symbol_Constant_String &rhs) const { return m_value > rhs.m_value; }
  bool Symbol_Constant_String::operator>=(const Symbol_Constant_String &rhs) const { return m_value >= rhs.m_value; }

  bool Symbol_Constant_String::operator>(const Symbol_Constant_Float &) const { return true; }
  bool Symbol_Constant_String::operator>=(const Symbol_Constant_Float &) const { return true; }
  bool Symbol_Constant_String::operator>(const Symbol_Constant_Int &) const { return true; }
  bool Symbol_Constant_String::operator>=(const Symbol_Constant_Int &) const { return true; }
  bool Symbol_Constant_String::operator<(const Symbol_Constant_Identifier &) const { return true; }
  bool Symbol_Constant_String::operator<=(const Symbol_Constant_Identifier &) const { return true; }
  bool Symbol_Constant_String::operator<(const Symbol_Variable &) const { return true; }
  bool Symbol_Constant_String::operator<=(const Symbol_Variable &) const { return true; }

  bool Symbol_Constant_String::operator==(const char * value_) const { return value_ == m_value; }

  size_t Symbol_Constant_String::hash() const {
    return std::hash<std::string>()(m_value);
  }

  std::ostream & Symbol_Constant_String::print(std::ostream &os) const {
    if (!m_value.empty() && m_value.find_first_of(" \t<>@()") == std::string::npos)
      return os << m_value;
    else
      return os << '|' << m_value << '|';
  }

  std::ostream & Symbol_Constant_String::print_contents(std::ostream &os) const {
    return os << m_value;
  }


  Symbol_Constant_Identifier::Symbol_Constant_Identifier(const std::string &value_) : m_value(value_) {}
  Symbol_Constant_Identifier::Symbol_Constant_Identifier(std::string &&value_) : m_value(std::move(value_)) {}

  const char * Symbol_Constant_Identifier::get_value() const { return m_value.c_str(); }

  Symbol_Constant_Identifier * Symbol_Constant_Identifier::clone() const { return new Symbol_Constant_Identifier(get_value()); }

  bool Symbol_Constant_Identifier::operator==(const Symbol &rhs) const { return rhs == *this; }
  bool Symbol_Constant_Identifier::operator!=(const Symbol &rhs) const { return rhs != *this; }
  bool Symbol_Constant_Identifier::operator>(const Symbol &rhs) const { return rhs < *this; }
  bool Symbol_Constant_Identifier::operator>=(const Symbol &rhs) const { return rhs <= *this; }
  bool Symbol_Constant_Identifier::operator<(const Symbol &rhs) const { return rhs > *this; }
  bool Symbol_Constant_Identifier::operator<=(const Symbol &rhs) const { return rhs >= *this; }

  bool Symbol_Constant_Identifier::operator==(const Symbol_Constant_Identifier &rhs) const { return m_value == rhs.m_value; }
  bool Symbol_Constant_Identifier::operator!=(const Symbol_Constant_Identifier &rhs) const { return m_value != rhs.m_value; }
  bool Symbol_Constant_Identifier::operator<(const Symbol_Constant_Identifier &rhs) const { return m_value < rhs.m_value; }
  bool Symbol_Constant_Identifier::operator<=(const Symbol_Constant_Identifier &rhs) const { return m_value <= rhs.m_value; }
  bool Symbol_Constant_Identifier::operator>(const Symbol_Constant_Identifier &rhs) const { return m_value > rhs.m_value; }
  bool Symbol_Constant_Identifier::operator>=(const Symbol_Constant_Identifier &rhs) const { return m_value >= rhs.m_value; }

  bool Symbol_Constant_Identifier::operator>(const Symbol_Constant_Float &) const { return true; }
  bool Symbol_Constant_Identifier::operator>=(const Symbol_Constant_Float &) const { return true; }
  bool Symbol_Constant_Identifier::operator>(const Symbol_Constant_Int &) const { return true; }
  bool Symbol_Constant_Identifier::operator>=(const Symbol_Constant_Int &) const { return true; }
  bool Symbol_Constant_Identifier::operator>(const Symbol_Constant_String &) const { return true; }
  bool Symbol_Constant_Identifier::operator>=(const Symbol_Constant_String &) const { return true; }
  bool Symbol_Constant_Identifier::operator<(const Symbol_Variable &) const { return true; }
  bool Symbol_Constant_Identifier::operator<=(const Symbol_Variable &) const { return true; }

  bool Symbol_Constant_Identifier::operator==(const char * value_) const { return value_ == m_value; }

  size_t Symbol_Constant_Identifier::hash() const {
    return std::hash<std::string>()(m_value);
  }

  std::ostream & Symbol_Constant_Identifier::print(std::ostream &os) const {
    return os << '@' << m_value;
  }

  std::ostream & Symbol_Constant_Identifier::print_contents(std::ostream &os) const {
    return os << '@' << m_value;
  }


  Symbol_Variable::Symbol_Variable(const char * value_) : m_value(value_) {}

  const char * Symbol_Variable::get_value() const { return m_value.c_str(); }

  Symbol_Variable * Symbol_Variable::clone() const { return new Symbol_Variable(m_value.c_str()); }

  bool Symbol_Variable::operator==(const Symbol &rhs) const { return rhs == *this; }
  bool Symbol_Variable::operator!=(const Symbol &rhs) const { return rhs != *this; }
  bool Symbol_Variable::operator>(const Symbol &rhs) const { return rhs < *this; }
  bool Symbol_Variable::operator>=(const Symbol &rhs) const { return rhs <= *this; }
  bool Symbol_Variable::operator<(const Symbol &rhs) const { return rhs > *this; }
  bool Symbol_Variable::operator<=(const Symbol &rhs) const { return rhs >= *this; }

  bool Symbol_Variable::operator==(const Symbol_Variable &rhs) const { return m_value == rhs.m_value; }
  bool Symbol_Variable::operator!=(const Symbol_Variable &rhs) const { return m_value != rhs.m_value; }
  bool Symbol_Variable::operator<(const Symbol_Variable &rhs) const { return m_value < rhs.m_value; }
  bool Symbol_Variable::operator<=(const Symbol_Variable &rhs) const { return m_value <= rhs.m_value; }
  bool Symbol_Variable::operator>(const Symbol_Variable &rhs) const { return m_value > rhs.m_value; }
  bool Symbol_Variable::operator>=(const Symbol_Variable &rhs) const { return m_value >= rhs.m_value; }

  bool Symbol_Variable::operator>(const Symbol_Constant_Float &) const { return true; }
  bool Symbol_Variable::operator>=(const Symbol_Constant_Float &) const { return true; }
  bool Symbol_Variable::operator>(const Symbol_Constant_Int &) const { return true; }
  bool Symbol_Variable::operator>=(const Symbol_Constant_Int &) const { return true; }
  bool Symbol_Variable::operator>(const Symbol_Constant_String &) const { return true; }
  bool Symbol_Variable::operator>=(const Symbol_Constant_String &) const { return true; }
  bool Symbol_Variable::operator>(const Symbol_Constant_Identifier &) const { return true; }
  bool Symbol_Variable::operator>=(const Symbol_Constant_Identifier &) const { return true; }

  bool Symbol_Variable::operator==(const char * value_) const { return value_ == m_value; }

  size_t Symbol_Variable::hash() const {
    return std::hash<std::string>()(m_value);
  }

  std::ostream & Symbol_Variable::print(std::ostream &os) const {
    os.put('<');
    os << m_value;
    os.put('>');
    return os;
  }

  std::ostream & Symbol_Variable::print_contents(std::ostream &) const {
    abort();
  }

}

bool operator==(const double lhs, const Zeni::Rete::Symbol &rhs) { return rhs == lhs; }
bool operator==(const int64_t lhs, const Zeni::Rete::Symbol &rhs) { return rhs == lhs; }
bool operator==(const char * lhs, const Zeni::Rete::Symbol &rhs) { return rhs == lhs; }

std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Symbol &symbol) {
  return symbol.print(os);
}
