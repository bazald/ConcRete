#include "Zeni/Rete/Token_Beta.hpp"

#include <cassert>
#include <iostream>
#include <sstream>

namespace Zeni {

  namespace Rete {

    Token_Beta::Token_Beta(const std::shared_ptr<const Token> &first, const std::shared_ptr<const Token> &second)
      : Token(first->size() + second->size()),
      m_first(first),
      m_second(second)
    {
    }

    void Token_Beta::print(std::ostream &os) const {
      assert(m_first);
      assert(m_second);
      m_first->print(os);
      m_second->print(os);
    }

    const std::shared_ptr<const Symbol> & Token_Beta::operator[](const Token_Index &index) const {
      assert(index.token_row >= 0);
      assert(index.token_row < size());

      const auto first_size = m_first->size();
      if (index.token_row < first_size)
        return (*m_first)[index];
      else
        return (*m_second)[Token_Index(index.rete_row, index.token_row - first_size, index.column)];
    }

  }

}
