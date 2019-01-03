#ifndef ZENI_RETE_NODE_KEY_HPP
#define ZENI_RETE_NODE_KEY_HPP

#include "Zeni/Concurrency/Container/Hash_Trie.hpp"
#include "Symbol.hpp"
#include "Variable_Binding.hpp"

namespace Zeni::Rete {
  class Node_Key;
  class Node_Key_Symbol_0;
  class Node_Key_Symbol_1;
  class Node_Key_Symbol_2;
  class Node_Key_Multisym;
}

namespace Zeni::Concurrency {
  template class ZENI_RETE_LINKAGE Hash_Trie<std::shared_ptr<const Rete::Node_Key>, hash_deref<Rete::Node_Key>, compare_deref_eq>;
}

namespace std {
  template class ZENI_RETE_LINKAGE weak_ptr<Zeni::Rete::Node_Key>;
#ifdef _MSC_VER
  struct ZENI_RETE_LINKAGE _Container_base12;
  template class ZENI_RETE_LINKAGE _Vector_val<std::_Simple_types<std::shared_ptr<const Zeni::Rete::Node_Key>>>;
  template class ZENI_RETE_LINKAGE _Compressed_pair<std::allocator<std::shared_ptr<const Zeni::Rete::Node_Key>>, std::_Vector_val<std::_Simple_types<std::shared_ptr<const Zeni::Rete::Node_Key>>>, true>;
  template class ZENI_RETE_LINKAGE vector<std::shared_ptr<const Zeni::Rete::Node_Key>>;
#endif
}

namespace Zeni::Rete {

  class Node_Key_Null;

  class ZENI_RETE_LINKAGE Node_Key : public std::enable_shared_from_this<Node_Key> {
    Node_Key(const Node_Key &) = delete;
    Node_Key & operator=(const Node_Key &) = delete;

  protected:
    Node_Key() {}

  public:
    virtual size_t hash() const = 0;

    virtual bool operator==(const Node_Key &rhs) const = 0;

    bool operator!=(const Node_Key &rhs) const { return !(*this == rhs); }

    virtual bool operator==(const Node_Key_Symbol_0 &rhs) const;
    virtual bool operator==(const Node_Key_Symbol_1 &rhs) const;
    virtual bool operator==(const Node_Key_Symbol_2 &rhs) const;
    virtual bool operator==(const Node_Key_Null &rhs) const;
    virtual bool operator==(const Node_Key_Multisym &rhs) const;

    virtual bool is_contained_by(const Node_Key &rhs) const = 0;

    virtual bool contains(const Node_Key_Symbol_0 &rhs) const;
    virtual bool contains(const Node_Key_Symbol_1 &rhs) const;
    virtual bool contains(const Node_Key_Symbol_2 &rhs) const;
    virtual bool contains(const Node_Key_Null &rhs) const;
    virtual bool contains(const Node_Key_Multisym &rhs) const;
  };

  class ZENI_RETE_LINKAGE Node_Key_Symbol_0 : public Node_Key {
    Node_Key_Symbol_0(const Node_Key_Symbol_0 &) = delete;
    Node_Key_Symbol_0 & operator=(const Node_Key_Symbol_0 &) = delete;

    Node_Key_Symbol_0(const std::shared_ptr<const Symbol> symbol);

  public:
    static std::shared_ptr<const Node_Key_Symbol_0> Create(const std::shared_ptr<const Symbol> symbol);

    size_t hash() const override;

    bool operator==(const Node_Key &rhs) const override;
    bool operator==(const Node_Key_Symbol_0 &rhs) const override;

    bool is_contained_by(const Node_Key &rhs) const override;
    bool contains(const Node_Key_Symbol_0 &rhs) const override;

    const std::shared_ptr<const Symbol> symbol;
  };

  class ZENI_RETE_LINKAGE Node_Key_Symbol_1 : public Node_Key {
    Node_Key_Symbol_1(const Node_Key_Symbol_1 &) = delete;
    Node_Key_Symbol_1 & operator=(const Node_Key_Symbol_1 &) = delete;

    Node_Key_Symbol_1(const std::shared_ptr<const Symbol> symbol);

  public:
    static std::shared_ptr<const Node_Key_Symbol_1> Create(const std::shared_ptr<const Symbol> symbol);

    size_t hash() const override;

    bool operator==(const Node_Key &rhs) const override;
    bool operator==(const Node_Key_Symbol_1 &rhs) const override;

    bool is_contained_by(const Node_Key &rhs) const override;
    bool contains(const Node_Key_Symbol_1 &rhs) const override;

    const std::shared_ptr<const Symbol> symbol;
  };

  class ZENI_RETE_LINKAGE Node_Key_Symbol_2 : public Node_Key {
    Node_Key_Symbol_2(const Node_Key_Symbol_2 &) = delete;
    Node_Key_Symbol_2 & operator=(const Node_Key_Symbol_2 &) = delete;

    Node_Key_Symbol_2(const std::shared_ptr<const Symbol> symbol);

  public:
    static std::shared_ptr<const Node_Key_Symbol_2> Create(const std::shared_ptr<const Symbol> symbol);

    size_t hash() const override;

    bool operator==(const Node_Key &rhs) const override;
    bool operator==(const Node_Key_Symbol_2 &rhs) const override;

    bool is_contained_by(const Node_Key &rhs) const override;
    bool contains(const Node_Key_Symbol_2 &rhs) const override;

    const std::shared_ptr<const Symbol> symbol;
  };

  class ZENI_RETE_LINKAGE Node_Key_Null : public Node_Key {
    Node_Key_Null(const Node_Key_Null &) = delete;
    Node_Key_Null & operator=(const Node_Key_Null &) = delete;

    Node_Key_Null();

  public:
    static std::shared_ptr<const Node_Key_Null> Create();

    size_t hash() const override;

    bool operator==(const Node_Key &rhs) const override;
    bool operator==(const Node_Key_Null &rhs) const override;

    bool is_contained_by(const Node_Key &rhs) const override;
    bool contains(const Node_Key_Null &rhs) const override;
  };

  class ZENI_RETE_LINKAGE Node_Key_Multisym : public Node_Key {
    Node_Key_Multisym(const Node_Key_Multisym &) = delete;
    Node_Key_Multisym & operator=(const Node_Key_Multisym &) = delete;

  public:
    typedef Concurrency::Hash_Trie<std::shared_ptr<const Node_Key>, hash_deref<Node_Key>, compare_deref_eq> Node_Key_Symbol_Trie;

  private:
    Node_Key_Multisym(const Node_Key_Symbol_Trie &symbols);
    Node_Key_Multisym(Node_Key_Symbol_Trie &&symbols);

  public:
    static std::shared_ptr<const Node_Key_Multisym> Create(const Node_Key_Symbol_Trie &symbols);
    static std::shared_ptr<const Node_Key_Multisym> Create(Node_Key_Symbol_Trie &&symbols);

    size_t hash() const override;

    bool operator==(const Node_Key &rhs) const override;
    bool operator==(const Node_Key_Multisym &rhs) const override;

    bool is_contained_by(const Node_Key &rhs) const override;
    bool contains(const Node_Key_Symbol_0 &rhs) const override;
    bool contains(const Node_Key_Symbol_1 &rhs) const override;
    bool contains(const Node_Key_Symbol_2 &rhs) const override;

    const Node_Key_Symbol_Trie symbols;
  };

}

namespace std {
  template <> struct hash<Zeni::Rete::Node_Key> {
    size_t operator()(const Zeni::Rete::Node_Key &node_key) const {
      return node_key.hash();
    }
  };

  template <> struct hash<Zeni::Rete::Node_Key_Symbol_0> {
    size_t operator()(const Zeni::Rete::Node_Key_Symbol_0 &node_key) const {
      return node_key.hash();
    }
  };

  template <> struct hash<Zeni::Rete::Node_Key_Symbol_1> {
    size_t operator()(const Zeni::Rete::Node_Key_Symbol_1 &node_key) const {
      return node_key.hash();
    }
  };

  template <> struct hash<Zeni::Rete::Node_Key_Symbol_2> {
    size_t operator()(const Zeni::Rete::Node_Key_Symbol_2 &node_key) const {
      return node_key.hash();
    }
  };
}

#endif
