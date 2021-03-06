#ifndef ZENI_UTILITY_HPP
#define ZENI_UTILITY_HPP

#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

namespace Zeni {

  class compare_deref_eq {
  public:
    template <typename Ptr1, typename Ptr2>
    bool operator()(const Ptr1 &lhs, const Ptr2 &rhs) const noexcept {
      return *lhs == *rhs;
    }
  };

  class compare_deref_lt {
  public:
    template <typename Ptr1, typename Ptr2>
    bool operator()(const Ptr1 &lhs, const Ptr2 &rhs) const noexcept {
      return *lhs < *rhs;
    }
  };

  template <typename Type1, typename Type2, int64_t(Type1::*MemFun)(const Type2 &) const>
  class compare_deref_memfun_lt {
  public:
    template <typename Ptr1, typename Ptr2>
    bool operator()(const Ptr1 &lhs, const Ptr2 &rhs) const noexcept {
      return std::bind(MemFun, &*lhs, std::cref(*rhs))() < 0;
    }
  };

  inline size_t hash_combine(const size_t &prev_h, const size_t &new_val) noexcept {
    return prev_h * 31 + new_val;
  }

  class compare_container_deref_eq {
  public:
    template <typename CONTAINER>
    bool operator()(const CONTAINER &lhs, const CONTAINER &rhs) const noexcept {
      if (lhs.size() != rhs.size())
        return false;
      for (auto lt = lhs.cbegin(), rt = rhs.cbegin(), lend = lhs.cend(); lt != lend; ++lt, ++rt) {
        if (**lt != **rt)
          return false;
      }
      return true;
    }
  };

  class compare_deref_container_deref_eq {
  public:
    template <typename CONTAINER>
    bool operator()(const CONTAINER &lhs, const CONTAINER &rhs) const noexcept {
      if (lhs->size() != rhs->size())
        return false;
      for (auto lt = lhs->cbegin(), rt = rhs->cbegin(), lend = lhs->cend(); lt != lend; ++lt, ++rt) {
        if (**lt != **rt)
          return false;
      }
      return true;
    }
  };

  class compare_container_deref_lt {
  public:
    template <typename CONTAINER>
    bool operator()(const CONTAINER &lhs, const CONTAINER &rhs) const noexcept {
      if (lhs.size() < rhs.size())
        return true;
      if (lhs.size() > rhs.size())
        return false;
      for (auto lt = lhs.cbegin(), rt = rhs.cbegin(), lend = lhs.cend(); lt != lend; ++lt, ++rt) {
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
    size_t operator()(const CONTAINER &container, const HASHER &hasher) const noexcept {
      size_t h = 0;
      for (const auto &entry : container)
        h = hash_combine(h, hasher(entry));
      return h;
    }

    template <typename CONTAINER>
    size_t operator()(const CONTAINER &container) const noexcept {
      return (*this)(container, std::hash<T>());
    }
  };

  template <typename T>
  class hash_container_deref {
  public:
    template <typename CONTAINER, typename HASHER>
    size_t operator()(const CONTAINER &container, const HASHER &hasher = std::hash<T>()) const noexcept {
      size_t h = 0;
      for (const auto &entry : container)
        h = hash_combine(h, hasher(*entry));
      return h;
    }

    template <typename CONTAINER>
    size_t operator()(const CONTAINER &container) const noexcept {
      return (*this)(container, std::hash<T>());
    }
  };

  template <typename T>
  class hash_deref {
  public:
    template <typename Ptr>
    size_t operator()(const Ptr &ptr) const noexcept {
      return std::hash<T>()(*ptr);
    }
  };

  template <typename T>
  class hash_deref_first {
  public:
    template <typename PairPtr>
    size_t operator()(const PairPtr &pp) const noexcept {
      return std::hash<T>()(*pp.first);
    }
  };

  class nullhash {
  public:
    size_t operator()(const size_t &val) const noexcept {
      return val;
    }
  };

//  template <typename Type>
//  constexpr std::string_view type_name() {
//    using namespace std;
//#ifdef __clang__
//    string_view p = __PRETTY_FUNCTION__;
//    return string_view(p.data() + 34, p.size() - 34 - 1);
//#elif defined(__GNUC__)
//    string_view p = __PRETTY_FUNCTION__;
//#  if __cplusplus < 201402
//    return string_view(p.data() + 36, p.size() - 36 - 1);
//#  else
//    return string_view(p.data() + 49, p.find(';', 49) - 49);
//#  endif
//#elif defined(_MSC_VER)
//    string_view p = __FUNCSIG__;
//    return string_view(p.data() + 84, p.size() - 84 - 7);
//#endif
//  }

/// Begin STATIC_WARNING implementation

#if defined(__GNUC__)
#define DEPRECATE(foo, msg) foo __attribute__((deprecated(msg)))
#elif defined(_MSC_VER)
#define DEPRECATE(foo, msg) __declspec(deprecated(msg)) foo
#else
#error This compiler is not supported
#endif

#define ZENI_PP_CAT1(x,y) x##y
#define ZENI_PP_CAT(x,y) ZENI_PP_CAT1(x,y)

  namespace Detail
  {
    struct true_type {};
    struct false_type {};
    template <int test> struct Converter : public true_type {};
    template <> struct Converter<0> : public false_type {};
  }

#define ZENI_STATIC_WARNING(cond, msg) \
    struct ZENI_PP_CAT(static_warning,__LINE__) { \
      DEPRECATE(void _(::Zeni::Detail::false_type const& ),msg) {}; \
      void _(::Zeni::Detail::true_type const& ) {}; \
      ZENI_PP_CAT(static_warning,__LINE__)() {_(::Zeni::Detail::Converter<(cond)>());} \
    }

  // Note: using STATIC_WARNING_TEMPLATE changes the meaning of a program in a small way.
  // It introduces a member/variable declaration.  This means at least one byte of space
  // in each structure/class instantiation.  STATIC_WARNING should be preferred in any 
  // non-template situation.
  //  'token' must be a program-wide unique identifier.
#define ZENI_STATIC_WARNING_TEMPLATE(token, cond, msg) \
    ZENI_STATIC_WARNING(cond, msg) PP_CAT(PP_CAT(_localvar_, token),__LINE__)

  /// End STATIC_WARNING implementation

  std::string to_string(const double &number) noexcept;

}

namespace std {

  template <typename T, size_t N>
  struct hash<array<T, N>> {
    size_t operator()(const array<T, N> &a) const noexcept {
      hash<T> hasher;
      return Zeni::hash_container<T>()(a, hasher);
    }
  };

  template <typename T1, typename T2>
  struct hash<pair<T1, T2>> {
    size_t operator()(const pair<T1, T2> &p) const noexcept {
      return Zeni::hash_combine(hash<T1>()(p.first), hash<T2>()(p.second));
    }
  };

  template <typename Key, typename Hash, typename Pred, typename Alloc>
  struct hash<unordered_set<Key, Hash, Pred, Alloc>> {
    size_t operator()(const unordered_set<Key, Hash, Pred, Alloc> &us) const noexcept {
      hash<Key> hasher;
      return Zeni::hash_container<Key>()(us, hasher);
    }
  };

  template <typename T, typename Alloc>
  struct hash<vector<T, Alloc>> {
    size_t operator()(const vector<T, Alloc> &v) const noexcept {
      hash<T> hasher;
      return Zeni::hash_container<T>()(v, hasher);
    }
  };

}

#endif
