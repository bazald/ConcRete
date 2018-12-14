#ifndef ZENI_CONCURRENCY_MEMORY_POOL_IMPL_HPP
#define ZENI_CONCURRENCY_MEMORY_POOL_IMPL_HPP

#include "../../Utility.hpp"
#include "../Container/Ctrie.hpp"
#include "../Container/Intrusive_Stack.hpp"
#include "../Memory_Pool.hpp"
#include "Mallocator.hpp"

#include <mutex>
#include <unordered_map>

namespace Zeni::Concurrency {

  class Memory_Pool_Stack : public Allocated<Jemallocator<Memory_Pool_Stack>> {
  public:
    struct Node : Intrusive_Stack<Node>::Node {};

    struct Deleter {
      void operator()(Memory_Pool_Stack * ptr) const {
        je_free(ptr);
      }

      template <class U>
      void operator()(U* ptr) const {
        je_free(ptr);
      }
    };

    struct Equals {
      bool operator()(const Memory_Pool_Stack &lhs, const Memory_Pool_Stack &rhs) const {
        return lhs.key == rhs.key;
      }
      bool operator()(const Memory_Pool_Stack &lhs, const Memory_Pool_Key &rhs) const {
        return lhs.key == rhs;
      }
      bool operator()(const Memory_Pool_Key &lhs, const Memory_Pool_Stack &rhs) const {
        return lhs == rhs.key;
      }
      bool operator()(const Memory_Pool_Key &lhs, const Memory_Pool_Key &rhs) const {
        return lhs == rhs;
      }
    };

    struct Hasher {
      size_t operator()(const Memory_Pool_Stack &stack) const {
        return stack.hash();
      }
      size_t operator()(const Memory_Pool_Key &key) const {
        return hash_combine(std::hash<size_t>()(key.first), std::hash<std::align_val_t>()(key.second));
      }
    };

    Memory_Pool_Stack()
      : stack(std::allocate_shared<Intrusive_Stack<Node, Deleter>>(Jemallocator_Aligned<Intrusive_Stack<Node, Deleter>, std::align_val_t(ZENI_CONCURRENCY_DESTRUCTIVE_INTERFERENCE_SIZE)>()))
    {
    }

    Memory_Pool_Stack(const Memory_Pool_Key &key)
      : key(key),
      stack(std::allocate_shared<Intrusive_Stack<Node, Deleter>>(Jemallocator_Aligned<Intrusive_Stack<Node, Deleter>, std::align_val_t(ZENI_CONCURRENCY_DESTRUCTIVE_INTERFERENCE_SIZE)>()))
    {
    }

    Memory_Pool_Stack(Memory_Pool_Key &&key)
      : key(std::move(key)),
      stack(std::allocate_shared<Intrusive_Stack<Node, Deleter>>(Jemallocator_Aligned<Intrusive_Stack<Node, Deleter>, std::align_val_t(ZENI_CONCURRENCY_DESTRUCTIVE_INTERFERENCE_SIZE)>()))
    {
    }

    size_t hash() const {
      return hash_combine(std::hash<size_t>()(key.first), std::hash<std::align_val_t>()(key.second));
    }

    bool operator==(const Memory_Pool_Stack &rhs) const {
      return key == rhs.key;
    }

    const Memory_Pool_Key key = Memory_Pool_Key();
    const std::shared_ptr<Intrusive_Stack<Node, Deleter>> stack = nullptr;
  };

  class Memory_Pool_Impl {
    Memory_Pool_Impl(const Memory_Pool_Impl &rhs) = delete;
    Memory_Pool_Impl & operator=(const Memory_Pool_Impl &rhs) = delete;

    Memory_Pool_Impl() noexcept;
    ~Memory_Pool_Impl() noexcept;

  public:
    static Memory_Pool_Impl & get() noexcept;

    /// Free any cached memory blocks.
    void clear() noexcept;

    /// Free any cached memory blocks from the last time we were in the current epoch.
    void rotate() noexcept;

    /// Get a cached memory block or allocate one as needed.
    void * allocate(size_t size) noexcept;

    /// Get a cached memory block or allocate one as needed.
    void * allocate(size_t size, const std::align_val_t alignment) noexcept;

    /// Return a memory block to be cached (and eventually freed).
    void release(void * const ptr, size_t size) noexcept;

    /// Return a memory block to be cached (and eventually freed).
    void release(void * const ptr, size_t size, const std::align_val_t alignment) noexcept;

  private:
    Intrusive_Stack<Memory_Pool_Stack::Node, Memory_Pool_Stack::Deleter> * get_stack(const Memory_Pool_Key &key) noexcept;
    Intrusive_Stack<Memory_Pool_Stack::Node, Memory_Pool_Stack::Deleter> * get_backup_stack(const Memory_Pool_Key &key) noexcept;
    Intrusive_Stack<Memory_Pool_Stack::Node, Memory_Pool_Stack::Deleter> * get_backup_stack_2(const Memory_Pool_Key &key) noexcept;
    void fill(void * const dest, const size_t size, const uint32_t pattern) noexcept;

    typedef Ctrie<Memory_Pool_Stack, Memory_Pool_Stack::Hasher, Memory_Pool_Stack::Equals, uint32_t, Jemallocator<char>> Trie;
    Trie m_freed_0;
    Trie m_freed_1;
    Trie m_freed_2;
  };

}

#endif
