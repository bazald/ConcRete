#ifndef ZENI_RETE_ANTIABLE_SET_HPP
#define ZENI_RETE_ANTIABLE_SET_HPP

#include <cinttypes>
#include <memory>
#include <unordered_map>

namespace Zeni::Rete {

  template <
    typename Key,                                                   // unordered_set::key_type/value_type
    typename Hash = std::hash<Key>,                                 // unordered_set::hasher
    typename Pred = std::equal_to<Key>,                             // unordered_set::key_equal
    typename Alloc = std::allocator<std::pair<const Key, int64_t>>> // unordered_set::allocator_type>
  class Antiable_Set {
    typedef std::unordered_map<Key, int64_t, Hash, Pred, Alloc> Values;

  public:
    typedef Key value_type;
    typedef const value_type & reference;

    class const_iterator {
    public:
      typedef std::forward_iterator_tag iterator_category;
      typedef Key value_type;
      typedef const value_type & reference;

      const_iterator(const typename Values::const_iterator &it, const typename Values::const_iterator &iend) : m_it(it), m_iend(iend) {}
      const_iterator(const typename Values::const_iterator &it, typename Values::const_iterator &&iend) : m_it(it), m_iend(std::move(iend)) {}
      const_iterator(typename Values::const_iterator &&it, const typename Values::const_iterator &iend) : m_it(std::move(it)), m_iend(iend) {}
      const_iterator(typename Values::const_iterator &&it, typename Values::const_iterator &&iend) : m_it(std::move(it)), m_iend(std::move(iend)) {}

      const_iterator(const const_iterator &rhs) : m_it(rhs.m_it) {}
      const_iterator(const_iterator &&rhs) : m_it(std::move(rhs.m_it)) {}

      const_iterator & operator=(const const_iterator &rhs) {
        m_it = rhs.m_it;
        return *this;
      }

      const_iterator & operator=(const_iterator &&rhs) {
        m_it = std::move(rhs.m_it);
        return *this;
      }

      reference operator*() const { return m_it->first; }

      const_iterator next() const {
        return ++const_iterator(*this);
      }

      const_iterator & operator++() {
        do {
          ++m_it;
        } while (m_it != m_iend && m_it->second < 0);
        return *this;
      }
      const_iterator operator++(int) {
        const_iterator rv(*this);
        do {
          ++m_it;
        } while (m_it != m_iend && m_it->second < 0);
        return rv;
      }

      bool operator==(const const_iterator &rhs) const {
        return m_it == rhs.m_it;
      }
      bool operator!=(const const_iterator &rhs) const {
        return m_it != rhs.m_it;
      }

    private:
      typename Values::const_iterator m_it;
      const typename Values::const_iterator m_iend;
    };

    const_iterator cbegin() const {
      auto it = m_values.cbegin();
      const auto iend = m_values.cend();
      while (it != iend && it->second < 0)
        ++it;
      return const_iterator(it, iend);
    }

    const_iterator cend() const {
      return const_iterator(m_values.cend(), m_values.cend());
    }

    const_iterator begin() const {
      return cbegin();
    }

    const_iterator end() const {
      return cend();
    }

    const_iterator find(const Key &key) const {
      const auto it = m_values.find(key);
      const auto iend = m_values.cend();
      return const_iterator(it != iend && it->second > 0 ? it : iend, iend);
    }

    bool empty() const {
      return m_size == 0;
    }

    size_t size() const {
      return m_size;
    }

    void clear() {
      m_values.clear();
    }

    /// Returns true if this is the first insertion of 'value', otherwise false
    bool try_emplace(const value_type &value) {
      const auto found = m_values.try_emplace(value, 1);

      if (found.second) {
        ++m_size;
        return true;
      }

      if (++found.first->second == 0)
        m_values.erase(found.first);

      return false;
    }

    /// Returns true if this is the first insertion of 'value', otherwise false
    bool try_emplace(value_type &&value) {
      const auto found = m_values.try_emplace(std::move(value), 1);

      if (found.second) {
        ++m_size;
        return true;
      }

      if (++found.first->second == 0)
        m_values.erase(found.first);

      return false;
    }

    /// Returns true if this was the last instance of 'value', otherwise false
    bool try_erase(const value_type &value) {
      const auto found = m_values.try_emplace(value, -1);

      if (found.second || --found.first->second != 0)
        return false;

      m_values.erase(found.first);
      --m_size;

      return true;
    }

    /// Returns true if this was the last instance of 'value', otherwise false
    bool try_erase(value_type &&value) {
      const auto found = m_values.try_emplace(std::move(value), -1);

      if (found.second || --found.first->second != 0)
        return false;

      m_values.erase(found.first);
      --m_size;

      return true;
    }

  private:
    Values m_values;
    size_t m_size = 0;
  };

}

#endif
