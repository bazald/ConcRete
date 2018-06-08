#ifndef ZENI_RETE_DEBUG_COUNTERS_HPP
#define ZENI_RETE_DEBUG_COUNTERS_HPP

#ifndef NDEBUG
#define ZENI_RETE_USE_DEBUG_COUNTERS
#endif

#include "Linkage.hpp"

#include <iosfwd>

#ifdef ZENI_RETE_USE_DEBUG_COUNTERS

#include "Zeni/Concurrency/Atomic.hpp"

#include <array>

namespace Zeni::Rete::Debug_Counters {

  extern ZENI_RETE_LINKAGE Zeni::Concurrency::Atomic_int64_t<true> g_node_increments;
  extern ZENI_RETE_LINKAGE Zeni::Concurrency::Atomic_int64_t<true> g_try_increment_child_counts;
  extern ZENI_RETE_LINKAGE Zeni::Concurrency::Atomic_int64_t<true> g_connect_gates_received;
  extern ZENI_RETE_LINKAGE Zeni::Concurrency::Atomic_int64_t<true> g_connect_outputs_received;
  extern ZENI_RETE_LINKAGE Zeni::Concurrency::Atomic_int64_t<true> g_decrement_children_received;
  extern ZENI_RETE_LINKAGE Zeni::Concurrency::Atomic_int64_t<true> g_disconnect_gates_received;
  extern ZENI_RETE_LINKAGE Zeni::Concurrency::Atomic_int64_t<true> g_disconnect_output_and_decrements_received;
  extern ZENI_RETE_LINKAGE Zeni::Concurrency::Atomic_int64_t<true> g_disconnect_output_but_nodecrements_received;
  extern ZENI_RETE_LINKAGE Zeni::Concurrency::Atomic_int64_t<true> g_empties_received;
  extern ZENI_RETE_LINKAGE Zeni::Concurrency::Atomic_int64_t<true> g_nonempties_received;
  extern ZENI_RETE_LINKAGE Zeni::Concurrency::Atomic_int64_t<true> g_tokens_inserted;
  extern ZENI_RETE_LINKAGE Zeni::Concurrency::Atomic_int64_t<true> g_tokens_removed;
  extern ZENI_RETE_LINKAGE std::array<Zeni::Concurrency::Atomic_int64_t<true>, 8> g_extra;

}

#define DEBUG_COUNTER_INCREMENT(COUNTER, AMOUNT) Zeni::Rete::Debug_Counters::COUNTER.fetch_add(AMOUNT)
#define DEBUG_COUNTER_DECREMENT(COUNTER, AMOUNT) Zeni::Rete::Debug_Counters::COUNTER.fetch_sub(AMOUNT)

#else

#define DEBUG_COUNTER_INCREMENT(COUNTER, AMOUNT)
#define DEBUG_COUNTER_DECREMENT(COUNTER, AMOUNT)

#endif

namespace Zeni::Rete::Debug_Counters {

  ZENI_RETE_LINKAGE void print(std::ostream &os);

  ZENI_RETE_LINKAGE void reset();

}

#endif
