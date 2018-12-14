#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>  
#endif

#include "Zeni/Concurrency/Internal/Memory_Pool_Impl.hpp"
#include "Zeni/Concurrency/Internal/Worker_Threads_Impl.hpp"

#ifdef _WIN32
#include "jemalloc/jemalloc.h"
#endif

namespace Zeni::Concurrency {

  Memory_Pool_Impl::Memory_Pool_Impl() noexcept
  {
#ifdef _MSC_VER
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    //_CrtSetBreakAlloc(167);
#endif
  }

  Memory_Pool_Impl::~Memory_Pool_Impl() noexcept {
    clear();
  }

  Memory_Pool_Impl & Memory_Pool_Impl::get() noexcept {
    static thread_local Memory_Pool_Impl g_memory_pool;
    return g_memory_pool;
  }

  void Memory_Pool_Impl::clear() noexcept {
    m_freed_0.clear();
    m_freed_1.clear();
    m_freed_2.clear();
  }

  void Memory_Pool_Impl::rotate() noexcept {
    const int32_t epoch = Worker_Threads_Impl::get_epoch();

    if (epoch == 0)
      m_freed_0.clear();
    else if (epoch == 1)
      m_freed_1.clear();
    else //if(epoch == 2)
      m_freed_2.clear();
  }

  void * Memory_Pool_Impl::allocate(size_t size) noexcept {
    if (size < sizeof(Memory_Pool_Stack::Node))
      size = sizeof(Memory_Pool_Stack::Node);

    auto stack = get_stack(Memory_Pool_Key(size, std::align_val_t()));

    void * ptr = stack->try_pop();
#ifndef NDEBUG
    if (ptr)
      fill(ptr, size, 0xFA57F00D);
#endif

    if (!ptr) {
      stack = get_backup_stack(Memory_Pool_Key(size, std::align_val_t()));

      ptr = stack ? stack->try_pop() : nullptr;
#ifndef NDEBUG
      if (ptr)
        fill(ptr, size, 0xFA57F00D);
#endif

      if (!ptr) {
        stack = get_backup_stack_2(Memory_Pool_Key(size, std::align_val_t()));

        ptr = stack ? stack->try_pop() : nullptr;
#ifndef NDEBUG
        if (ptr)
          fill(ptr, size, 0xFA57F00D);
#endif

        if (!ptr) {
          ptr = je_malloc(size);
#ifndef NDEBUG
          if (ptr)
            fill(ptr, size, 0xED1B13BF);
#endif
        }
      }
    }

    return ptr;
  }

  void * Memory_Pool_Impl::allocate(size_t size, const std::align_val_t alignment) noexcept {
    if (size < sizeof(Memory_Pool_Stack::Node))
      size = sizeof(Memory_Pool_Stack::Node);

    auto stack = get_stack(Memory_Pool_Key(size, alignment));

    void * ptr = stack->try_pop();
#ifndef NDEBUG
    if (ptr)
      fill(ptr, size, 0xFA57F00D);
#endif

    if (!ptr) {
      stack = get_backup_stack(Memory_Pool_Key(size, std::align_val_t()));

      ptr = stack ? stack->try_pop() : nullptr;
#ifndef NDEBUG
      if (ptr)
        fill(ptr, size, 0xFA57F00D);
#endif

      if (!ptr) {
        stack = get_backup_stack_2(Memory_Pool_Key(size, std::align_val_t()));

        ptr = stack ? stack->try_pop() : nullptr;
#ifndef NDEBUG
        if (ptr)
          fill(ptr, size, 0xFA57F00D);
#endif

        if (!ptr) {
          ptr = je_aligned_alloc(size_t(alignment), size);
#ifndef NDEBUG
          if (ptr)
            fill(ptr, size, 0xED1B13BF);
#endif
        }
      }
    }

    return ptr;
  }

  void Memory_Pool_Impl::release(void * const ptr, size_t size) noexcept {
    assert(ptr);

    if (size < sizeof(Memory_Pool_Stack::Node))
      size = sizeof(Memory_Pool_Stack::Node);

    const auto stack = get_stack(Memory_Pool_Key(size, std::align_val_t()));

    const auto node = reinterpret_cast<Memory_Pool_Stack::Node *>(ptr);

#ifndef NDEBUG
    fill(ptr, size, 0xFA57F00D);
#endif

    stack->push(node);
  }

  void Memory_Pool_Impl::release(void * const ptr, size_t size, const std::align_val_t alignment) noexcept {
    assert(ptr);

    if (size < sizeof(Memory_Pool_Stack::Node))
      size = sizeof(Memory_Pool_Stack::Node);

    const auto stack = get_stack(Memory_Pool_Key(size, alignment));

    const auto node = reinterpret_cast<Memory_Pool_Stack::Node *>(ptr);

#ifndef NDEBUG
    fill(ptr, size, 0xFA57F00D);
#endif

    stack->push(node);
  }

  Intrusive_Stack<Memory_Pool_Stack::Node, Memory_Pool_Stack::Deleter> * Memory_Pool_Impl::get_stack(const Memory_Pool_Key &key) noexcept {
    const int32_t epoch = Worker_Threads_Impl::get_epoch();
    auto &freed = epoch == 0 ? m_freed_0 : epoch == 1 ? m_freed_1 : /*epoch == 2*/ m_freed_2;

    const auto[result1, found1] = freed.lookup(key);
    if (result1 == Trie::Result::Found)
      return found1.stack.get();

    const auto[result2, inserted2, replaced2] = freed.insert(Memory_Pool_Stack(key));
    assert(result2 == Trie::Result::First_Insertion);
    return inserted2.stack.get();
  }

  Intrusive_Stack<Memory_Pool_Stack::Node, Memory_Pool_Stack::Deleter> * Memory_Pool_Impl::get_backup_stack(const Memory_Pool_Key &key) noexcept {
    const int32_t epoch = Worker_Threads_Impl::get_epoch();
    auto &freed = epoch == 0 ? m_freed_2 : epoch == 1 ? m_freed_0 : /*epoch == 2*/ m_freed_1;

    const auto[result, found] = freed.lookup(key);
    return found.stack.get();
  }

  Intrusive_Stack<Memory_Pool_Stack::Node, Memory_Pool_Stack::Deleter> * Memory_Pool_Impl::get_backup_stack_2(const Memory_Pool_Key &key) noexcept {
    const int32_t epoch = Worker_Threads_Impl::get_epoch();
    auto &freed = epoch == 0 ? m_freed_1 : epoch == 1 ? m_freed_2 : /*epoch == 2*/ m_freed_0;

    const auto[result, found] = freed.lookup(key);
    return found.stack.get();
  }

  void Memory_Pool_Impl::fill(void * const dest, const size_t size, const uint32_t pattern) noexcept {
    unsigned char * dd = reinterpret_cast<unsigned char *>(dest);
    const unsigned char * const dend = dd + size;

    while (dend - dd > 3) {
      *reinterpret_cast<uint32_t *>(dd) = pattern;
      dd += 4;
    }
    if (dd == dend)
      return;
    *dd = pattern >> 24;
    if (++dd == dend)
      return;
    *dd = (pattern >> 16) & 0xFF;
    if (++dd == dend)
      return;
    *dd = (pattern >> 8) & 0xFF;
  }

}
