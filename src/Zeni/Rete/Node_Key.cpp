#include "Zeni/Rete/Node_Key.hpp"

#include <cassert>

namespace Zeni::Rete {

  bool Node_Key::operator==(const Node_Key_Symbol &) const {
    return false;
  }

  bool Node_Key::operator==(const Node_Key_Null &) const {
    return false;
  }

  bool Node_Key::operator==(const Node_Key_01 &) const {
    return false;
  }

  bool Node_Key::operator==(const Node_Key_02 &) const {
    return false;
  }

  bool Node_Key::operator==(const Node_Key_12 &) const {
    return false;
  }

  bool Node_Key::operator==(const Node_Key_Multisym &) const {
    return false;
  }

  bool Node_Key::contains(const Node_Key_Symbol &) const {
    return false;
  }

  bool Node_Key::contains(const Node_Key_Null &) const {
    return false;
  }

  bool Node_Key::contains(const Node_Key_01 &) const {
    return false;
  }

  bool Node_Key::contains(const Node_Key_02 &) const {
    return false;
  }

  bool Node_Key::contains(const Node_Key_12 &) const {
    return false;
  }

  bool Node_Key::contains(const Node_Key_Multisym &) const {
    return false;
  }

  Node_Key_Symbol::Node_Key_Symbol(const std::shared_ptr<const Symbol> symbol_)
    : symbol(symbol_)
  {
  }

  std::shared_ptr<const Node_Key_Symbol> Node_Key_Symbol::Create(const std::shared_ptr<const Symbol> symbol) {
    class Friendly_Node_Key_Symbol : public Node_Key_Symbol {
    public:
      Friendly_Node_Key_Symbol(const std::shared_ptr<const Symbol> symbol) : Node_Key_Symbol(symbol) {}
    };

    return std::make_shared<Friendly_Node_Key_Symbol>(symbol);
  }

  size_t Node_Key_Symbol::hash() const {
    return symbol->hash();
  }

  bool Node_Key_Symbol::operator==(const Node_Key &rhs) const {
    return rhs == *this;
  }

  bool Node_Key_Symbol::operator==(const Node_Key_Symbol &rhs) const {
    return *symbol == *rhs.symbol;
  }

  bool Node_Key_Symbol::is_contained_by(const Node_Key &rhs) const {
    return rhs.contains(*this);
  }

  bool Node_Key_Symbol::contains(const Node_Key_Symbol &rhs) const {
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

  Node_Key_01::Node_Key_01()
  {
  }

  std::shared_ptr<const Node_Key_01> Node_Key_01::Create() {
    class Friendly_Node_Key_01 : public Node_Key_01 {
    public:
      Friendly_Node_Key_01() {}
    };

    static std::shared_ptr<const Node_Key_01> g_node_key_01 = std::make_shared<Friendly_Node_Key_01>();
    return g_node_key_01;
  }

  size_t Node_Key_01::hash() const {
    return std::hash<size_t>()(2);
  }

  bool Node_Key_01::operator==(const Node_Key &rhs) const {
    return rhs == *this;
  }

  bool Node_Key_01::operator==(const Node_Key_01 &) const {
    return true;
  }

  bool Node_Key_01::is_contained_by(const Node_Key &rhs) const {
    return rhs.contains(*this);
  }

  bool Node_Key_01::contains(const Node_Key_01 &) const {
    return true;
  }

  Node_Key_02::Node_Key_02()
  {
  }

  std::shared_ptr<const Node_Key_02> Node_Key_02::Create() {
    class Friendly_Node_Key_02 : public Node_Key_02 {
    public:
      Friendly_Node_Key_02() {}
    };

    static std::shared_ptr<const Node_Key_02> g_node_key_02 = std::make_shared<Friendly_Node_Key_02>();
    return g_node_key_02;
  }

  size_t Node_Key_02::hash() const {
    return std::hash<size_t>()(3);
  }

  bool Node_Key_02::operator==(const Node_Key &rhs) const {
    return rhs == *this;
  }

  bool Node_Key_02::operator==(const Node_Key_02 &) const {
    return true;
  }

  bool Node_Key_02::is_contained_by(const Node_Key &rhs) const {
    return rhs.contains(*this);
  }

  bool Node_Key_02::contains(const Node_Key_02 &) const {
    return true;
  }

  Node_Key_12::Node_Key_12()
  {
  }

  std::shared_ptr<const Node_Key_12> Node_Key_12::Create() {
    class Friendly_Node_Key_12 : public Node_Key_12 {
    public:
      Friendly_Node_Key_12() {}
    };

    static std::shared_ptr<const Node_Key_12> g_node_key_12 = std::make_shared<Friendly_Node_Key_12>();
    return g_node_key_12;
  }

  size_t Node_Key_12::hash() const {
    return std::hash<size_t>()(4);
  }

  bool Node_Key_12::operator==(const Node_Key &rhs) const {
    return rhs == *this;
  }

  bool Node_Key_12::operator==(const Node_Key_12 &) const {
    return true;
  }

  bool Node_Key_12::is_contained_by(const Node_Key &rhs) const {
    return rhs.contains(*this);
  }

  bool Node_Key_12::contains(const Node_Key_12 &) const {
    return true;
  }

  Node_Key_Multisym::Node_Key_Multisym(const Node_Key_Symbol_Trie &symbols_)
    : symbols(symbols_)
  {
    assert(symbols.size() > 1);
  }

  Node_Key_Multisym::Node_Key_Multisym(Node_Key_Symbol_Trie &&symbols_)
    : symbols(std::move(symbols_))
  {
    assert(symbols.size() > 1);
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
    return hash_container_deref<Node_Key_Symbol>()(symbols);
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

  bool Node_Key_Multisym::contains(const Node_Key_Symbol &rhs) const {
    return symbols.looked_up(&rhs) != nullptr;
  }

}
