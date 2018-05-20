#ifndef ZENI_UTILITY_H
#define ZENI_UTILITY_H

#include <string>
#include <unordered_set>

namespace Zeni {

  class compare_deref_eq {
  public:
    template <typename Ptr1, typename Ptr2>
    bool operator()(const Ptr1 &lhs, const Ptr2 &rhs) const {
      return *lhs == *rhs;
    }
  };

  class compare_deref_lt {
  public:
    template <typename Ptr1, typename Ptr2>
    bool operator()(const Ptr1 &lhs, const Ptr2 &rhs) const {
      return *lhs < *rhs;
    }
  };

  template <typename Type1, typename Type2, int64_t(Type1::*MemFun)(const Type2 &) const>
  class compare_deref_memfun_lt {
  public:
    template <typename Ptr1, typename Ptr2>
    bool operator()(const Ptr1 &lhs, const Ptr2 &rhs) const {
      return std::bind(MemFun, &*lhs, std::cref(*rhs))() < 0;
    }
  };

  inline size_t hash_combine(const size_t &prev_h, const size_t &new_val) {
    return prev_h * 31 + new_val;
  }

  class compare_container_deref_eq {
  public:
    template <typename CONTAINER>
    bool operator()(const CONTAINER &lhs, const CONTAINER &rhs) const {
      if (lhs.size() != rhs.size())
        return false;
      for (auto lt = lhs.begin(), rt = rhs.begin(), lend = lhs.end(); lt != lend; ++lt, ++rt) {
        if (**lt != **rt)
          return false;
      }
      return true;
    }
  };

  class compare_container_deref_lt {
  public:
    template <typename CONTAINER>
    bool operator()(const CONTAINER &lhs, const CONTAINER &rhs) const {
      if (lhs.size() < rhs.size())
        return true;
      if (lhs.size() > rhs.size())
        return false;
      for (auto lt = lhs.begin(), rt = rhs.begin(), lend = lhs.end(); lt != lend; ++lt, ++rt) {
        if (**lt < **rt)
          return true;
        else if (**lt > **rt)
          return false;
      }
      return false;
    }
  };

  template <typename T>
  class hash_container {
  public:
    template <typename CONTAINER, typename HASHER>
    size_t operator()(const CONTAINER &container, const HASHER &hasher) const {
      size_t h = 0;
      for (const auto &entry : container)
        h = hash_combine(h, hasher(entry));
      return h;
    }

    template <typename CONTAINER>
    size_t operator()(const CONTAINER &container) const {
      return (*this)(container, std::hash<T>());
    }
  };

  template <typename T>
  class hash_container_deref {
  public:
    template <typename CONTAINER, typename HASHER>
    size_t operator()(const CONTAINER &container, const HASHER &hasher = std::hash<T>()) const {
      size_t h = 0;
      for (const auto &entry : container)
        h = hash_combine(h, hasher(*entry));
      return h;
    }

    template <typename CONTAINER>
    size_t operator()(const CONTAINER &container) const {
      return (*this)(container, std::hash<T>());
    }
  };

  template <typename T>
  class hash_deref {
  public:
    template <typename Ptr>
    size_t operator()(const Ptr &ptr) const {
      return std::hash<T>()(*ptr);
    }
  };

  template <typename T>
  class hash_deref_first {
  public:
    template <typename PairPtr>
    size_t operator()(const PairPtr &pp) const {
      return std::hash<T>()(*pp.first);
    }
  };

  class nullhash {
  public:
    size_t operator()(const size_t &val) const {
      return val;
    }
  };

  std::string to_string(const double &number);

}

namespace std {

  template <typename T, size_t N>
  struct hash<array<T, N>> {
    size_t operator()(const array<T, N> &a) const {
      hash<T> hasher;
      return Zeni::hash_container<T>()(a, hasher);
    }
  };

  template <typename T1, typename T2>
  struct hash<pair<T1, T2>> {
    size_t operator()(const pair<T1, T2> &p) const {
      return Zeni::hash_combine(hash<T1>()(p.first), hash<T2>()(p.second));
    }
  };

  template <typename Key, typename Hash, typename Pred, typename Alloc>
  struct hash<unordered_set<Key, Hash, Pred, Alloc>> {
    size_t operator()(const unordered_set<Key, Hash, Pred, Alloc> &us) const {
      hash<Key> hasher;
      return Zeni::hash_container<Key>()(us, hasher);
    }
  };

  template <typename T, typename Alloc>
  struct hash<vector<T, Alloc>> {
    size_t operator()(const vector<T, Alloc> &v) const {
      hash<T> hasher;
      return Zeni::hash_container<T>()(v, hasher);
    }
  };

}

#endif
