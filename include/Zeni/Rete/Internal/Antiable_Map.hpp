#ifndef ZENI_RETE_ANTIABLE_MAP_HPP
#define ZENI_RETE_ANTIABLE_MAP_HPP

#include <cinttypes>
#include <memory>
#include <unordered_map>

namespace Zeni::Rete {

  template <
    typename Key,
    typename T,
    typename Hash = std::hash<Key>,                                               // unordered_set::hasher
    typename Pred = std::equal_to<Key>,                                           // unordered_set::key_equal
    typename Alloc = std::allocator<std::pair<const Key, std::pair<int64_t, T>>>> // unordered_set::allocator_type>
  class Antiable_Map {
    typedef std::unordered_map<Key, std::pair<int64_t, T>, Hash, Pred, Alloc> Values;

  public:
    typedef std::pair<Key, T> value_type;
    typedef const value_type & reference;

    class const_iterator {
    public:
      typedef std::forward_iterator_tag iterator_category;
      typedef std::pair<Key, T> value_type;
      typedef std::pair<Key &, T &> reference;

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

      reference operator*() const { return reference(m_it->first, m_it->second.second); }

      const_iterator prev() const { return m_it.prev(); }
      const_iterator next() const { return m_it.next(); }

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

    size_t size() const {
      return m_size;
    }

    void clear() {
      m_values.clear();
    }

    /// Returns true if this is the first insertion of 'value', otherwise false
    std::pair<bool, T> try_emplace(const value_type &value) {
      const auto found = m_values.try_emplace(value.first, std::make_pair(1, value.second));

      if (found.second) {
        ++m_size;
        return std::make_pair(true, found.first->second.second);
      }

      if (++found.first->second.first != 0)
        return std::make_pair(false, found.first->second.second);

      const auto removed = std::move(found.first->second.second);
      m_values.erase(found.first);

      return std::make_pair(false, removed);
    }

    /// Returns true if this is the first insertion of 'value', otherwise false
    std::pair<bool, T> try_emplace(value_type &&value) {
      const auto found = m_values.try_emplace(value.first, std::make_pair(1, std::move(value.second)));

      if (found.second) {
        ++m_size;
        return std::make_pair(true, found.first->second.second);
      }

      if (++found.first->second.first != 0)
        return std::make_pair(false, found.first->second.second);

      const auto removed = std::move(found.first->second.second);
      m_values.erase(found.first);

      return std::make_pair(false, removed);
    }

    /// Returns true if this was the last instance of 'value', otherwise false
    std::pair<bool, T> try_erase(const Key &key) {
      const auto found = m_values.try_emplace(key, std::make_pair(-1, T()));

      if (found.second || --found.first->second.first != 0)
        return std::make_pair(false, found.first->second.second);

      const auto value = std::move(found.first->second.second);
      m_values.erase(found.first);
      --m_size;

      return std::make_pair(true, value);
    }

    /// Returns true if this was the last instance of 'value', otherwise false
    std::pair<bool, T> try_erase(Key &&key) {
      const auto found = m_values.try_emplace(std::move(key), std::make_pair(-1, T()));

      if (found.second || --found.first->second.first != 0)
        return std::make_pair(false, found.first->second.second);

      const auto value = std::move(found.first->second.second);
      m_values.erase(found.first);
      --m_size;

      return std::make_pair(true, value);
    }

  private:
    Values m_values;
    size_t m_size = 0;
  };

}

#endif
