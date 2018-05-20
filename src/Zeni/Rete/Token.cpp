#include "Zeni/Rete/Token.hpp"

#include <cassert>
#include <iostream>
#include <sstream>

namespace Zeni {

  namespace Rete {

    Token::Token()
      : m_size(0),
      m_hashval(size_t(-1))
    {
    }

    Token::Token(const std::shared_ptr<const WME> &wme)
      : m_size(1),
      m_wme(wme),
      m_hashval(std::hash<WME>()(*m_wme))
    {
    }

    Token::Token(const std::shared_ptr<const Token> &first, const std::shared_ptr<const Token> &second)
      : m_first(first),
      m_second(second),
      m_size(first->m_size + second->m_size),
      m_hashval(hash_combine(std::hash<std::shared_ptr<const Token>>()(m_first), std::hash<std::shared_ptr<const Token>>()(m_second)))
    {
      assert(first->m_size);
      assert(second->m_size);
    }

    std::shared_ptr<const Token> Token::shared() const { return shared_from_this(); }
    std::shared_ptr<Token> Token::shared() { return shared_from_this(); }

    const std::shared_ptr<const WME> & Token::get_wme() const {
      assert(m_size == 1);
      return m_wme;
    }

    int64_t Token::size() const {
      return m_size;
    }

    bool Token::operator==(const Token &rhs) const {
      return m_first == rhs.m_first && m_second == rhs.m_second /*&& m_size == rhs.m_size*/ && m_wme == rhs.m_wme;
    }

    bool Token::operator!=(const Token &rhs) const {
      return m_first != rhs.m_first || m_second != rhs.m_second /*|| m_size != rhs.m_size*/ || m_wme != rhs.m_wme;
    }

    std::ostream & Token::print(std::ostream &os) const {
      if (m_size == 1) {
        assert(m_wme);
        os << *m_wme;
      }
      else if (m_size > 1) {
        assert(m_first);
        assert(m_second);
        m_first->print(os);
        m_second->print(os);
      }

      return os;
    }

    bool Token::eval_bindings(const bool &from_left, const Variable_Bindings &bindings, const Token &rhs, const bool &rhs_from_left) const {
      for (auto &binding : bindings) {
        if (*(*this)[from_left ? binding.first : binding.second] != *(rhs)[rhs_from_left ? binding.first : binding.second])
          return false;
      }
      return true;
    }

    size_t Token::hash_bindings(const bool &from_left, const Variable_Bindings &bindings) const {
      size_t hashval = 0;
      for (const auto &binding : bindings)
        hashval = hash_combine(hashval, (*this)[from_left ? binding.first : binding.second]->hash());
      return hashval;
    }

    const std::shared_ptr<const Symbol> & Token::operator[](const Token_Index &index) const {
      if (m_wme) {
        assert(index.token_row == 0);
        assert(index.column < 3);

        return m_wme->symbols[index.column];
      }
      else {
        assert(index.token_row < m_size);

        const auto first_size = m_first->m_size;
        if (index.token_row < first_size)
          return (*m_first)[index];
        else
          return (*m_second)[Token_Index(index.rete_row, index.token_row - first_size, index.column)];
      }
    }

    //std::pair<std::shared_ptr<const Variable_Indices>, std::shared_ptr<const Variable_Indices>> split_Variable_Indices(const Variable_Bindings &bindings, const std::shared_ptr<const Variable_Indices> &indices, const int64_t &offset) {
    //  //std::pair<std::shared_ptr<Variable_Indices>, std::shared_ptr<Variable_Indices>> rv =
    //  //  std::make_pair(std::make_shared<Variable_Indices>(), std::make_shared<Variable_Indices>());
    //  std::pair<std::shared_ptr<const Variable_Indices>, std::shared_ptr<Variable_Indices>> rv =
    //    std::make_pair(indices, std::make_shared<Variable_Indices>());
    //
    //  for(const auto &index : *indices) {
    //    if(index.second.first < offset)
    //      ;// (*rv.first)[index.first] = index.second;
    //    else
    //      rv.second->insert(std::make_pair(index.first, index.second));
    //  }
    //
    //  for(const auto &binding : bindings) {
    //    const auto found = find_if(rv.first->begin(), rv.first->end(), [&binding](const std::pair<std::string, Token_Index> &index)->bool {
    //      return index.second == binding.second;
    //    });
    //    assert(found != rv.first->end());
    //    rv.second->insert(std::make_pair(found->first, binding.second));
    //  }
    //
    //  return rv;
    //}

    std::string get_Variable_name(const std::shared_ptr<const Variable_Indices> &indices, const Token_Index &index) {
      //#ifdef DEBUG_OUTPUT
      //    std::cerr << std::endl << "Name " << index << " from " << *indices << std::endl;
      //#endif

      const auto found = std::find_if(indices->begin(), indices->end(), [index](const std::pair<std::string, Token_Index> &ind)->bool {
        return ind.second.rete_row == index.rete_row && ind.second.column == index.column;
      });
      //    assert(found != indices->end());
      if (found != indices->end())
        return found->first;

      std::ostringstream oss;
      oss << index;
      return oss.str();
    }

  }

}

std::ostream & operator<<(std::ostream &os, const Zeni::Rete::Token &token) {
  os << '{';
  token.print(os);
  os << '}';
  return os;
}
