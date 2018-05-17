#ifndef WME_TOKEN_H
#define WME_TOKEN_H

#include "wme.h"
#include "utility.h"
#include <cassert>

namespace Rete {

  class Rete_Node;
  class WME_Token;
  typedef std::shared_ptr<const WME_Token> WME_Token_Ptr_C;
  typedef std::shared_ptr<WME_Token> WME_Token_Ptr;
  typedef std::pair<WME_Token_Index, WME_Token_Index> WME_Binding;
  typedef std::set<WME_Binding> WME_Bindings;

  class RETE_LINKAGE WME_Token : public std::enable_shared_from_this<WME_Token>, public Zeni::Pool_Allocator<WME_Token> {
  public:
    WME_Token();
    WME_Token(const WME_Ptr_C &wme);
    WME_Token(const WME_Token_Ptr_C &first, const WME_Token_Ptr_C &second);

    WME_Token_Ptr_C shared() const {return shared_from_this();}
    WME_Token_Ptr shared() {return shared_from_this();}

    const WME_Ptr_C & get_wme() const {
      assert(m_size == 1);
      return m_wme;
    }

    int64_t size() const {
      return m_size;
    }

    bool operator==(const WME_Token &rhs) const;
    bool operator!=(const WME_Token &rhs) const;

    size_t get_hash() const {return m_hashval;}

    bool eval_bindings(const bool &from_left, const WME_Bindings &bindings, const WME_Token &rhs, const bool &rhs_from_left) const;
    size_t hash_bindings(const bool &from_left, const WME_Bindings &bindings) const;

    std::ostream & print(std::ostream &os) const;

    const Symbol_Ptr_C & operator[](const WME_Token_Index &index) const;

  private:
    std::pair<WME_Token_Ptr_C, WME_Token_Ptr_C> m_wme_token;
    int64_t m_size;

    WME_Ptr_C m_wme;

    size_t m_hashval;
  };

  Variable_Indices_Ptr_C bind_Variable_Indices(const WME_Bindings &bindings, const Variable_Indices_Ptr_C &indices, const Rete_Node &left, const Rete_Node &right);
  std::string get_Variable_name(const Variable_Indices_Ptr_C &indices, const WME_Token_Index &index);

}

RETE_LINKAGE std::ostream & operator<<(std::ostream &os, const Rete::WME_Binding &binding);
RETE_LINKAGE std::ostream & operator<<(std::ostream &os, const Rete::WME_Bindings &bindings);
RETE_LINKAGE std::ostream & operator<<(std::ostream &os, const Rete::WME_Token &wme_token);

namespace std {
  template <> struct hash<Rete::WME_Token> {
    size_t operator()(const Rete::WME_Token &wme_token) const {
      return wme_token.get_hash();
    }
  };
}

#endif
