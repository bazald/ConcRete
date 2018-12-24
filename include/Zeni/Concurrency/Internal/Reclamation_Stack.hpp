#ifndef ZENI_CONCURRENCY_RECLAMATION_STACK_HPP
#define ZENI_CONCURRENCY_RECLAMATION_STACK_HPP

#include "Linkage.hpp"

#include <atomic>
#include <cassert>
#include <cstdint>

namespace Zeni::Concurrency {

  class Reclamation_Stack {
    Reclamation_Stack(const Reclamation_Stack &) = delete;
    Reclamation_Stack & operator=(const Reclamation_Stack &) = delete;

    ZENI_CONCURRENCY_LINKAGE Reclamation_Stack() noexcept;

    ZENI_CONCURRENCY_LINKAGE ~Reclamation_Stack() noexcept;

  public:
    struct Node {
      ZENI_CONCURRENCY_LINKAGE virtual ~Node() = default;

      mutable const Node * reclamation_next = nullptr;
    };

    ZENI_CONCURRENCY_LINKAGE static Reclamation_Stack & get() noexcept;

    //int64_t size() const {
    //  return m_size.load(std::memory_order_relaxed);
    //}

    ZENI_CONCURRENCY_LINKAGE static void push(const Node * const node) noexcept;

    ZENI_CONCURRENCY_LINKAGE static void reclaim() noexcept;

    ZENI_CONCURRENCY_LINKAGE static void final_reclaim() noexcept;

  private:
    const Node * m_head = nullptr;
    const Node * m_head2 = nullptr;
    const Node * m_head3 = nullptr;
    //ZENI_CONCURRENCY_CACHE_ALIGN std::atomic_int64_t m_size = 0;
    bool m_middestruction = false;
    bool m_destroyed = false;
  };

}

#endif
