#ifndef ZENI_RETE_ANTIABLE_SET_HPP
#define ZENI_RETE_ANTIABLE_SET_HPP

#include <cinttypes>
#include <unordered_map>

namespace Zeni::Rete {

  template <typename TYPE>
  class Antiable_Set {
    typedef std::unordered_map<TYPE, int64_t> Values;

  public:
    typedef std::forward_iterator_tag iterator_category;
    typedef TYPE value_type;
    typedef const value_type * pointer;
    typedef const value_type & reference;

    class const_iterator {
    public:
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
      pointer operator->() const { return m_it->first; }

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

    /// Returns true if this is the first insertion of 'value', otherwise false
    bool try_emplace(const TYPE &value) {
      const auto found = m_values.find(value);
      if (found != m_values.cend()) {
        if (++found->second == 0)
          m_values.erase(found);
        return false;
      }

      m_values.emplace(std::make_pair(value, 1));
      ++m_size;
      return true;
    }

    /// Returns true if this was the last insertion of 'value', otherwise false
    bool try_erase(const TYPE &value) {
      const auto found = m_values.find(value);
      if (found != m_values.cend()) {
        if (--found->second == 0) {
          m_values.erase(found);
          --m_size;
          return true;
        }
        else
          return false;
      }

      m_values.emplace(std::make_pair(value, -1));
      return false;
    }

    size_t size() const {
      return m_size;
    }

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

  private:
    Values m_values;
    size_t m_size = 0;
  };

}

#endif
