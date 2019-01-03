#include "Zeni/Rete/Node_Key.hpp"

#include <cassert>

namespace Zeni::Rete {

  bool Node_Key::operator==(const Node_Key_Symbol_0 &) const {
    return false;
  }

  bool Node_Key::operator==(const Node_Key_Symbol_1 &) const {
    return false;
  }

  bool Node_Key::operator==(const Node_Key_Symbol_2 &) const {
    return false;
  }

  bool Node_Key::operator==(const Node_Key_Null &) const {
    return false;
  }

  bool Node_Key::operator==(const Node_Key_Multisym &) const {
    return false;
  }

  bool Node_Key::contains(const Node_Key_Symbol_0 &) const {
    return false;
  }

  bool Node_Key::contains(const Node_Key_Symbol_1 &) const {
    return false;
  }

  bool Node_Key::contains(const Node_Key_Symbol_2 &) const {
    return false;
  }

  bool Node_Key::contains(const Node_Key_Null &) const {
    return false;
  }

  bool Node_Key::contains(const Node_Key_Multisym &) const {
    return false;
  }

  Node_Key_Symbol_0::Node_Key_Symbol_0(const std::shared_ptr<const Symbol> symbol_)
    : symbol(symbol_)
  {
  }

  std::shared_ptr<const Node_Key_Symbol_0> Node_Key_Symbol_0::Create(const std::shared_ptr<const Symbol> symbol) {
    class Friendly_Node_Key_Symbol_0 : public Node_Key_Symbol_0 {
    public:
      Friendly_Node_Key_Symbol_0(const std::shared_ptr<const Symbol> symbol) : Node_Key_Symbol_0(symbol) {}
    };

    return std::make_shared<Friendly_Node_Key_Symbol_0>(symbol);
  }

  size_t Node_Key_Symbol_0::hash() const {
    return symbol->hash();
  }

  bool Node_Key_Symbol_0::operator==(const Node_Key &rhs) const {
    return rhs == *this;
  }

  bool Node_Key_Symbol_0::operator==(const Node_Key_Symbol_0 &rhs) const {
    return *symbol == *rhs.symbol;
  }

  bool Node_Key_Symbol_0::is_contained_by(const Node_Key &rhs) const {
    return rhs.contains(*this);
  }

  bool Node_Key_Symbol_0::contains(const Node_Key_Symbol_0 &rhs) const {
    return *symbol == *rhs.symbol;
  }

  Node_Key_Symbol_1::Node_Key_Symbol_1(const std::shared_ptr<const Symbol> symbol_)
    : symbol(symbol_)
  {
  }

  std::shared_ptr<const Node_Key_Symbol_1> Node_Key_Symbol_1::Create(const std::shared_ptr<const Symbol> symbol) {
    class Friendly_Node_Key_Symbol_1 : public Node_Key_Symbol_1 {
    public:
      Friendly_Node_Key_Symbol_1(const std::shared_ptr<const Symbol> symbol) : Node_Key_Symbol_1(symbol) {}
    };

    return std::make_shared<Friendly_Node_Key_Symbol_1>(symbol);
  }

  size_t Node_Key_Symbol_1::hash() const {
    return symbol->hash();
  }

  bool Node_Key_Symbol_1::operator==(const Node_Key &rhs) const {
    return rhs == *this;
  }

  bool Node_Key_Symbol_1::operator==(const Node_Key_Symbol_1 &rhs) const {
    return *symbol == *rhs.symbol;
  }

  bool Node_Key_Symbol_1::is_contained_by(const Node_Key &rhs) const {
    return rhs.contains(*this);
  }

  bool Node_Key_Symbol_1::contains(const Node_Key_Symbol_1 &rhs) const {
    return *symbol == *rhs.symbol;
  }

  Node_Key_Symbol_2::Node_Key_Symbol_2(const std::shared_ptr<const Symbol> symbol_)
    : symbol(symbol_)
  {
  }

  std::shared_ptr<const Node_Key_Symbol_2> Node_Key_Symbol_2::Create(const std::shared_ptr<const Symbol> symbol) {
    class Friendly_Node_Key_Symbol_2 : public Node_Key_Symbol_2 {
    public:
      Friendly_Node_Key_Symbol_2(const std::shared_ptr<const Symbol> symbol) : Node_Key_Symbol_2(symbol) {}
    };

    return std::make_shared<Friendly_Node_Key_Symbol_2>(symbol);
  }

  size_t Node_Key_Symbol_2::hash() const {
    return symbol->hash();
  }

  bool Node_Key_Symbol_2::operator==(const Node_Key &rhs) const {
    return rhs == *this;
  }

  bool Node_Key_Symbol_2::operator==(const Node_Key_Symbol_2 &rhs) const {
    return *symbol == *rhs.symbol;
  }

  bool Node_Key_Symbol_2::is_contained_by(const Node_Key &rhs) const {
    return rhs.contains(*this);
  }

  bool Node_Key_Symbol_2::contains(const Node_Key_Symbol_2 &rhs) const {
    return *symbol == *rhs.symbol;
  }

  Node_Key_Null::Node_Key_Null()
  {
  }

  std::shared_ptr<const Node_Key_Null> Node_Key_Null::Create() {
    class Friendly_Node_Key_Null : public Node_Key_Null {
    public:
      Friendly_Node_Key_Null() {}
    };

    static std::shared_ptr<const Node_Key_Null> g_node_key_null = std::make_shared<Friendly_Node_Key_Null>();
    return g_node_key_null;
  }

  size_t Node_Key_Null::hash() const {
    return std::hash<size_t>()(1);
  }

  bool Node_Key_Null::operator==(const Node_Key &rhs) const {
    return rhs == *this;
  }

  bool Node_Key_Null::operator==(const Node_Key_Null &) const {
    return true;
  }

  bool Node_Key_Null::is_contained_by(const Node_Key &rhs) const {
    return rhs.contains(*this);
  }

  bool Node_Key_Null::contains(const Node_Key_Null &) const {
    return true;
  }

  Node_Key_Multisym::Node_Key_Multisym(const Node_Key_Symbol_Trie &symbols_)
    : symbols(symbols_)
  {
#ifndef NDEBUG
    assert(symbols.size() > 1);
    const auto sbegin = symbols.cbegin();
    const auto send = symbols.cend();
    if (sbegin != send) {
      auto st = sbegin;
      ++st;
      if (dynamic_cast<const Node_Key_Symbol_0 *>((*sbegin).get())) {
        for (; st != send; ++st)
          assert(dynamic_cast<const Node_Key_Symbol_0 *>((*st).get()));
      }
      else if (dynamic_cast<const Node_Key_Symbol_1 *>((*sbegin).get())) {
        for (; st != send; ++st)
          assert(dynamic_cast<const Node_Key_Symbol_1 *>((*st).get()));
      }
      else if (dynamic_cast<const Node_Key_Symbol_2 *>((*sbegin).get())) {
        for (; st != send; ++st)
          assert(dynamic_cast<const Node_Key_Symbol_2 *>((*st).get()));
      }
      else
        abort();
    }
#endif
  }

  Node_Key_Multisym::Node_Key_Multisym(Node_Key_Symbol_Trie &&symbols_)
    : symbols(std::move(symbols_))
  {
#ifndef NDEBUG
    assert(symbols.size() > 1);
    const auto sbegin = symbols.cbegin();
    const auto send = symbols.cend();
    if (sbegin != send) {
      auto st = sbegin;
      ++st;
      if (dynamic_cast<const Node_Key_Symbol_0 *>((*sbegin).get())) {
        for (; st != send; ++st)
          assert(dynamic_cast<const Node_Key_Symbol_0 *>((*st).get()));
      }
      else if (dynamic_cast<const Node_Key_Symbol_1 *>((*sbegin).get())) {
        for (; st != send; ++st)
          assert(dynamic_cast<const Node_Key_Symbol_1 *>((*st).get()));
      }
      else if (dynamic_cast<const Node_Key_Symbol_2 *>((*sbegin).get())) {
        for (; st != send; ++st)
          assert(dynamic_cast<const Node_Key_Symbol_2 *>((*st).get()));
      }
      else
        abort();
    }
#endif
  }

  std::shared_ptr<const Node_Key_Multisym> Node_Key_Multisym::Create(const Node_Key_Symbol_Trie &symbols) {
    class Friendly_Node_Key_Multisym : public Node_Key_Multisym {
    public:
      Friendly_Node_Key_Multisym(const Node_Key_Symbol_Trie symbols) : Node_Key_Multisym(symbols) {}
    };

    return std::make_shared<Friendly_Node_Key_Multisym>(symbols);
  }

  std::shared_ptr<const Node_Key_Multisym> Node_Key_Multisym::Create(Node_Key_Symbol_Trie &&symbols) {
    class Friendly_Node_Key_Multisym : public Node_Key_Multisym {
    public:
      Friendly_Node_Key_Multisym(Node_Key_Symbol_Trie &&symbols) : Node_Key_Multisym(std::move(symbols)) {}
    };

    return std::make_shared<Friendly_Node_Key_Multisym>(std::move(symbols));
  }

  size_t Node_Key_Multisym::hash() const {
    return hash_container_deref<Node_Key>()(symbols);
  }

  bool Node_Key_Multisym::operator==(const Node_Key &rhs) const {
    return rhs == *this;
  }

  bool Node_Key_Multisym::operator==(const Node_Key_Multisym &rhs) const {
    return compare_container_deref_eq()(symbols, rhs.symbols);
  }

  bool Node_Key_Multisym::is_contained_by(const Node_Key &) const {
    return false;
  }

  bool Node_Key_Multisym::contains(const Node_Key_Symbol_0 &rhs) const {
    return symbols.looked_up(&rhs) != nullptr;
  }

  bool Node_Key_Multisym::contains(const Node_Key_Symbol_1 &rhs) const {
    return symbols.looked_up(&rhs) != nullptr;
  }

  bool Node_Key_Multisym::contains(const Node_Key_Symbol_2 &rhs) const {
    return symbols.looked_up(&rhs) != nullptr;
  }

}
