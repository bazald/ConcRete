#include "wme_token.h"

#include "rete_node.h"

#include <iostream>

namespace Rete {

  WME_Token::WME_Token()
   : m_size(0),
   m_hashval(size_t(-1))
  {
  }

  WME_Token::WME_Token(const WME_Ptr_C &wme)
   : m_size(1),
   m_wme(wme),
   m_hashval(std::hash<WME>()(*m_wme))
  {
  }

  WME_Token::WME_Token(const WME_Token_Ptr_C &first, const WME_Token_Ptr_C &second)
   : m_wme_token(first, second),
   m_size(first->m_size + second->m_size),
   m_hashval(hash_combine(std::hash<WME_Token_Ptr_C>()(m_wme_token.first), std::hash<WME_Token_Ptr_C>()(m_wme_token.second)))
  {
    assert(first->m_size);
    assert(second->m_size);
  }

  bool WME_Token::operator==(const WME_Token &rhs) const {
    return m_wme_token == rhs.m_wme_token /*&& m_size == rhs.m_size*/ && m_wme == rhs.m_wme;
  }

  bool WME_Token::operator!=(const WME_Token &rhs) const {
    return m_wme_token != rhs.m_wme_token /*|| m_size != rhs.m_size*/ || m_wme != rhs.m_wme;
  }

  std::ostream & WME_Token::print(std::ostream &os) const {
    if(m_size == 1) {
      assert(m_wme);
      os << *m_wme;
    }
    else if(m_size > 1){
      assert(m_wme_token.first);
      assert(m_wme_token.second);
      m_wme_token.first->print(os);
      m_wme_token.second->print(os);
    }

    return os;
  }

  bool WME_Token::eval_bindings(const bool &from_left, const WME_Bindings &bindings, const WME_Token &rhs, const bool &rhs_from_left) const {
    for(auto &binding : bindings) {
      if(*(*this)[from_left ? binding.first : binding.second] != *(rhs)[rhs_from_left ? binding.first : binding.second])
        return false;
    }
    return true;
  }

  size_t WME_Token::hash_bindings(const bool &from_left, const WME_Bindings &bindings) const {
    size_t hashval = 0;
    for(const auto &binding : bindings)
      hashval = hash_combine(hashval, (*this)[from_left ? binding.first : binding.second]->hash());
    return hashval;
  }

  const Symbol_Ptr_C & WME_Token::operator[](const WME_Token_Index &index) const {
    if(m_wme) {
      assert(index.token_row == 0);
      assert(index.column < 3);

      return m_wme->symbols[index.column];
    }
    else {
      assert(index.token_row < m_size);

      const auto first_size = m_wme_token.first->m_size;
      if(index.token_row < first_size)
        return (*m_wme_token.first)[index];
      else
        return (*m_wme_token.second)[WME_Token_Index(index.rete_row, index.token_row - first_size, index.column)];
    }
  }

//  std::pair<Variable_Indices_Ptr_C, Variable_Indices_Ptr_C> split_Variable_Indices(const WME_Bindings &bindings, const Variable_Indices_Ptr_C &indices, const int64_t &offset) {
////    std::pair<Variable_Indices_Ptr, Variable_Indices_Ptr> rv =
////      std::make_pair(std::make_shared<Variable_Indices>(), std::make_shared<Variable_Indices>());
//    std::pair<Variable_Indices_Ptr_C, Variable_Indices_Ptr> rv =
//      std::make_pair(indices, std::make_shared<Variable_Indices>());
//
//    for(const auto &index : *indices) {
//      if(index.second.first < offset)
//        ;// (*rv.first)[index.first] = index.second;
//      else
//        rv.second->insert(std::make_pair(index.first, index.second));
//    }
//
//    for(const auto &binding : bindings) {
//      const auto found = find_if(rv.first->begin(), rv.first->end(), [&binding](const std::pair<std::string, WME_Token_Index> &index)->bool {
//        return index.second == binding.second;
//      });
//      assert(found != rv.first->end());
//      rv.second->insert(std::make_pair(found->first, binding.second));
//    }
//
//    return rv;
//  }

  Variable_Indices_Ptr_C bind_Variable_Indices(const WME_Bindings &bindings, const Variable_Indices_Ptr_C &indices, const Rete_Node &left, const Rete_Node &right) {
    const int64_t left_size = left.get_size();
    const int64_t left_token_size = left.get_token_size();
    const int64_t right_size = right.get_size();

    Variable_Indices_Ptr closure = std::make_shared<Variable_Indices>();

    for(const auto &index : *indices) {
      if(index.second.rete_row >= left_size && index.second.rete_row < left_size + right_size)
        closure->insert(std::make_pair(index.first, WME_Token_Index(index.second.rete_row - left_size, index.second.token_row - left_token_size, index.second.column)));
    }

    for(const auto &binding : bindings) {
      auto found = std::find_if(indices->begin(), indices->end(), [binding](const std::pair<std::string, WME_Token_Index> &ind)->bool {
        return ind.second.token_row == binding.first.token_row && ind.second.column == binding.first.column;
      });
      if(found != indices->end()) {
        closure->insert(std::make_pair(found->first, binding.second));
        continue;
      }

      found = std::find_if(closure->begin(), closure->end(), [binding](const std::pair<std::string, WME_Token_Index> &ind)->bool {
        return ind.second.token_row == binding.first.token_row && ind.second.column == binding.first.column;
      });
      if(found != closure->end())
        closure->insert(std::make_pair(found->first, binding.second));
    }

    return closure;
  }

  std::string get_Variable_name(const Variable_Indices_Ptr_C &indices, const WME_Token_Index &index) {
//#ifdef DEBUG_OUTPUT
//    std::cerr << std::endl << "Name " << index << " from " << *indices << std::endl;
//#endif

    const auto found = std::find_if(indices->begin(), indices->end(), [index](const std::pair<std::string, WME_Token_Index> &ind)->bool {
      return ind.second.rete_row == index.rete_row && ind.second.column == index.column;
    });
//    assert(found != indices->end());
    if(found != indices->end())
      return found->first;

    std::ostringstream oss;
    oss << index;
    return oss.str();
  }

}

std::ostream & operator<<(std::ostream &os, const Rete::WME_Binding &binding) {
  return os << binding.first << ':' << binding.second;
}

std::ostream & operator<<(std::ostream &os, const Rete::WME_Bindings &bindings) {
  if(bindings.empty())
    return os << "[]";
  os << '[';
  auto bt = bindings.begin();
  os << *bt++;
  while(bt != bindings.end())
    os << ',' << *bt++;
  os << ']';
  return os;
}

std::ostream & operator<<(std::ostream &os, const Rete::WME_Token &wme_token) {
  os << '{';
  wme_token.print(os);
  os << '}';
  return os;
}
